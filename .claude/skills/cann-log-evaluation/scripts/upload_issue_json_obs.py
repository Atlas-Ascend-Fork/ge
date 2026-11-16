#!/usr/bin/env python3
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""Validate and upload the dated Markdown/JSON bundle to dashboard OBS paths.

正式交付是日期化 Markdown 报告 + issue-tracking.json；Excel 是可选的附件。
历史 JSON（report_url 指向旧 .xlsx）仍可补传：校验、日期解析和默认报告源
都兼容 .md 与 .xlsx，上传顺序固定为 Markdown →（可选 Excel）→ JSON。
"""

import argparse
import hashlib
import json
import os
import posixpath
import sys
import tempfile
from pathlib import Path
from urllib.parse import quote

import report_artifacts


DEFAULT_BUCKET = "opencann-obs"
DEFAULT_SERVER = "https://obs.cn-north-4.myhuaweicloud.com"
DEFAULT_DOMAIN = "https://opencann-obs.obs.cn-north-4.myhuaweicloud.com"
TARGET_TEMPLATES = (
    "cann_exp/static/prod/A2_A3/agent_component_log/default/{date}/issue-tracking.json",
    "ann_exp/static/prod/A5/agent_component_log/default/{date}/issue-tracking.json",
)
REQUIRED_FIELDS = {
    "uuid",
    "error_type",
    "file",
    "component",
    "component_en",
    "repo",
    "branch",
    "line_number",
    "rule",
    "description",
    "severity",
    "status",
    "discovered_at",
    "responsible_team",
    "contact",
    "contact_en",
    "report_url",
}
VALID_SEVERITIES = {"P0", "P1", "P2"}
DEFAULT_REPORT_SUFFIX = "md"
REPORT_CONTENT_TYPES = {
    ".md": "text/markdown; charset=utf-8",
    ".xlsx": ("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"),
}
JSON_CONTENT_TYPE = "application/json; charset=utf-8"
DEFAULT_MULTIPART_THRESHOLD = 8 * 1024 * 1024
DEFAULT_PART_SIZE = 5 * 1024 * 1024
DEFAULT_MULTIPART_TASKS = 2


class UploadError(RuntimeError):
    """Raised when an OBS upload fails."""


def validate_date(value):
    return report_artifacts.validate_run_date(value)


def current_dashboard_date(now=None):
    """在实际上传时生成看板日期；传入 now 仅用于确定性测试。"""
    return report_artifacts.current_run_date(now)


def report_content_type(report_name_or_path):
    """按后缀返回报告对象的 content-type；只支持 .md 与 .xlsx。"""
    suffix = Path(str(report_name_or_path)).suffix.lower()
    try:
        return REPORT_CONTENT_TYPES[suffix]
    except KeyError as exc:
        raise ValueError(
            "报告文件必须是 .md 或 .xlsx：%s" % report_name_or_path
        ) from exc


def load_and_validate_json(path, run_date=None):
    path = Path(path).resolve()
    if not path.is_file():
        raise FileNotFoundError("找不到 JSON 文件：%s" % path)
    try:
        with path.open("r", encoding="utf-8") as handle:
            records = json.load(handle)
    except json.JSONDecodeError as exc:
        raise ValueError("JSON 格式错误：%s" % exc) from exc
    if not isinstance(records, list):
        raise ValueError("JSON 顶层必须是问题数组")
    # 正式交付指向 Markdown 主报告；历史 Excel JSON 仍然接受，但两者都必须是
    # 本轮日期生成的规范地址，避免跨日期误传。
    expected_reports = ()
    if run_date:
        run_date = validate_date(run_date)
        expected_reports = (
            report_artifacts.report_url(run_date, DEFAULT_REPORT_SUFFIX),
            report_artifacts.report_url(run_date, "xlsx"),
        )
    for index, record in enumerate(records, 1):
        if not isinstance(record, dict):
            raise ValueError("第 %d 条问题不是对象" % index)
        missing = sorted(REQUIRED_FIELDS - set(record))
        if missing:
            raise ValueError("第 %d 条问题缺少字段：%s" % (index, ", ".join(missing)))
        if record["severity"] not in VALID_SEVERITIES:
            raise ValueError("第 %d 条问题 severity 必须是 P0/P1/P2" % index)
        if (
            not isinstance(record["line_number"], int)
            or isinstance(record["line_number"], bool)
            or record["line_number"] < 0
        ):
            raise ValueError("第 %d 条问题 line_number 必须是非负整数" % index)
        if (
            not isinstance(record["error_type"], int)
            or isinstance(record["error_type"], bool)
            or record["error_type"] <= 0
        ):
            raise ValueError("第 %d 条问题 error_type 必须是正整数" % index)
        if not str(record["component"]).strip():
            raise ValueError("第 %d 条问题 component 不能为空" % index)
        if not str(record["responsible_team"]).strip():
            raise ValueError("第 %d 条问题 responsible_team 不能为空" % index)
        location = "%s:%d:%d" % (
            str(record.get("file") or ""),
            record["line_number"],
            record["error_type"],
        )
        expected_md5 = hashlib.md5(location.encode("utf-8")).hexdigest()
        if str(record.get("uuid") or "") != expected_md5:
            raise ValueError(
                "第 %d 条问题 uuid 必须等于文件名+行号+错误类型的 MD5" % index
            )
        if expected_reports and str(record["report_url"]) not in expected_reports:
            raise ValueError(
                "第 %d 条问题 report_url 必须是本轮 Markdown 或 Excel 报告地址："
                "%s / %s" % (index, expected_reports[0], expected_reports[1])
            )
    return path, len(records)


def report_date_from_json(path):
    """从 JSON 的 report_url 解析日期；兼容 .md 与旧 .xlsx 地址。"""
    with Path(path).open("r", encoding="utf-8") as handle:
        records = json.load(handle)
    dates = {
        report_artifacts.report_date(str(record.get("report_url") or ""))
        for record in records
    }
    if not dates:
        return None
    if len(dates) != 1:
        raise ValueError("JSON 中存在多个报告日期：%s" % ", ".join(sorted(dates)))
    return next(iter(dates))


def target_keys(run_date):
    run_date = validate_date(run_date)
    return [template.format(date=run_date) for template in TARGET_TEMPLATES]


def target_report_keys(run_date, report_name):
    """把报告放到每个 issue-tracking.json 的同级目录。

    接受 .md（主报告）与 .xlsx（历史/可选附件）文件名，但文件名中的日期
    必须与上传日期一致。
    """
    run_date = validate_date(run_date)
    name = Path(str(report_name)).name
    if name != str(report_name):
        raise ValueError("报告名必须是不含目录的文件名：%s" % report_name)
    try:
        report_date = report_artifacts.report_date(name)
    except ValueError as exc:
        raise ValueError(
            "报告名必须为 %s-YYYYMMDD.md 或 .xlsx：%s"
            % (report_artifacts.REPORT_PREFIX, name)
        ) from exc
    if report_date != run_date:
        raise ValueError("报告名日期必须与上传日期 %s 一致：%s" % (run_date, name))
    return [
        posixpath.join(posixpath.dirname(key), name) for key in target_keys(run_date)
    ]


def public_url(domain, key):
    return "%s/%s" % (domain.rstrip("/"), quote(key, safe="/"))


def validate_report_reference(issue_source, report_source):
    """校验报告与 JSON report_url 一致，返回 (报告路径, content-type)。

    默认要求 Markdown 主报告；显式传入 .xlsx（历史补传）时允许，并按后缀
    设置 content-type。
    """
    report = Path(report_source).resolve()
    if not report.is_file():
        raise FileNotFoundError("找不到报告文件：%s" % report)
    content_type = report_content_type(report.name)
    report_date = report_artifacts.report_date(report.name)
    suffix = report.suffix.lower().lstrip(".")
    expected_url = report_artifacts.report_url(report_date, suffix)
    with Path(issue_source).open("r", encoding="utf-8") as handle:
        records = json.load(handle)
    mismatches = [
        index
        for index, record in enumerate(records, 1)
        if str(record.get("report_url") or "") != expected_url
    ]
    if mismatches:
        raise ValueError(
            "JSON report_url 必须全部等于 %s，首个不匹配记录=%d"
            % (expected_url, mismatches[0])
        )
    return report, content_type


def json_report_urls(issue_source):
    """读取 JSON 中的 report_url 列表；格式异常时返回空列表。"""
    try:
        with Path(issue_source).open("r", encoding="utf-8") as handle:
            records = json.load(handle)
    except (OSError, json.JSONDecodeError):
        return []
    if not isinstance(records, list):
        return []
    return [
        str(record.get("report_url") or "")
        for record in records
        if isinstance(record, dict)
    ]


def default_report_source(issue_source, run_date):
    """按 JSON 同目录与上传日期查找报告；默认 Markdown，缺失时回退 Excel。

    历史 JSON 的 report_url 指向 .xlsx 时优先使用 Excel 附件，保证旧数据
    仍可补传；新 JSON（report_url 为 .md）优先使用 Markdown 主报告。
    """
    run_date = validate_date(run_date)
    directory = Path(issue_source).resolve().parent
    urls = json_report_urls(issue_source)
    preferred = DEFAULT_REPORT_SUFFIX
    if urls and all(url.lower().endswith(".xlsx") for url in urls):
        preferred = "xlsx"
    fallback = "xlsx" if preferred == DEFAULT_REPORT_SUFFIX else DEFAULT_REPORT_SUFFIX
    for suffix in (preferred, fallback):
        candidate = directory / report_artifacts.report_name(run_date, suffix)
        if candidate.is_file():
            return candidate
    return directory / report_artifacts.report_name(run_date, preferred)


def validate_excel_source(excel_source, run_date):
    """校验显式 Excel 附件存在、后缀与日期，返回绝对路径。"""
    excel = Path(excel_source).resolve()
    if not excel.is_file():
        raise FileNotFoundError("找不到 Excel 报告：%s" % excel)
    if excel.suffix.lower() != ".xlsx":
        raise ValueError("Excel 附件必须是 .xlsx 文件：%s" % excel)
    target_report_keys(run_date, excel.name)
    return excel


def credentials_from_environment():
    access_key = os.environ.get("OBS_ACCESS_KEY_ID") or os.environ.get("AccessKeyID")
    secret_key = os.environ.get("OBS_SECRET_ACCESS_KEY") or os.environ.get(
        "SecretAccessKey"
    )
    security_token = os.environ.get("OBS_SECURITY_TOKEN") or os.environ.get(
        "SecurityToken"
    )
    if not access_key or not secret_key:
        raise ValueError("请设置 OBS_ACCESS_KEY_ID 和 OBS_SECRET_ACCESS_KEY 环境变量")
    return access_key, secret_key, security_token


def load_obs_sdk():
    try:
        from obs import ObsClient, PutObjectHeader
    except ImportError as exc:
        raise RuntimeError(
            "缺少华为云 OBS SDK，请先运行 pip install -r requirements.txt"
        ) from exc
    return ObsClient, PutObjectHeader


def checkpoint_path(source, key):
    identity = hashlib.sha256((str(source) + "\0" + key).encode("utf-8")).hexdigest()[
        :16
    ]
    directory = Path(tempfile.gettempdir()) / "cann-log-evaluation-obs"
    directory.mkdir(parents=True, exist_ok=True)
    return directory / (identity + ".checkpoint")


def upload_one(
    client,
    bucket,
    key,
    source,
    headers,
    multipart_threshold=DEFAULT_MULTIPART_THRESHOLD,
    part_size=DEFAULT_PART_SIZE,
    multipart_tasks=DEFAULT_MULTIPART_TASKS,
):
    if source.stat().st_size < multipart_threshold:
        return client.putFile(bucket, key, str(source), None, headers)

    checkpoint = checkpoint_path(source, key)
    response = client.uploadFile(
        bucket,
        key,
        str(source),
        partSize=part_size,
        taskNum=multipart_tasks,
        enableCheckpoint=True,
        checkpointFile=str(checkpoint),
        checkSum=True,
        headers=headers,
        encoding_type="url",
    )
    status = getattr(response, "status", 0)
    if 200 <= status < 300 and checkpoint.exists():
        checkpoint.unlink()
    return response


def upload_objects(
    objects,
    bucket=DEFAULT_BUCKET,
    server=DEFAULT_SERVER,
    domain=DEFAULT_DOMAIN,
    access_key=None,
    secret_key=None,
    security_token=None,
    signature="auto",
    client_factory=None,
    header_factory=None,
    multipart_threshold=DEFAULT_MULTIPART_THRESHOLD,
    part_size=DEFAULT_PART_SIZE,
    multipart_tasks=DEFAULT_MULTIPART_TASKS,
):
    """按传入顺序上传对象；报告放前面可避免 JSON 先可见但链接未就绪。"""
    if access_key is None or secret_key is None:
        access_key, secret_key, environment_token = credentials_from_environment()
        security_token = security_token or environment_token
    if client_factory is None or header_factory is None:
        sdk_client, sdk_header = load_obs_sdk()
        client_factory = client_factory or sdk_client
        header_factory = header_factory or sdk_header

    client_options = {
        "access_key_id": access_key,
        "secret_access_key": secret_key,
        "server": server,
    }
    if security_token:
        client_options["security_token"] = security_token
    if signature != "auto":
        client_options["signature"] = signature
        client_options["is_signature_negotiation"] = False
        if signature == "v4":
            client_options["region"] = "cn-north-4"
    client = client_factory(**client_options)
    uploaded, failures = [], []
    try:
        for source, key, content_type in objects:
            url = public_url(domain, key)
            headers = header_factory()
            headers.contentType = content_type
            try:
                response = upload_one(
                    client,
                    bucket,
                    key,
                    Path(source),
                    headers,
                    multipart_threshold=multipart_threshold,
                    part_size=part_size,
                    multipart_tasks=multipart_tasks,
                )
            except Exception as exc:
                failures.append("%s: %s: %s" % (url, type(exc).__name__, exc))
                continue
            status = getattr(response, "status", 0)
            if status < 200 or status >= 300:
                error_code = getattr(response, "errorCode", "unknown")
                error_message = getattr(response, "errorMessage", "unknown")
                request_id = getattr(response, "requestId", "")
                failures.append(
                    "%s: status=%s code=%s message=%s request_id=%s"
                    % (url, status, error_code, error_message, request_id)
                )
                continue
            uploaded.append(url)
        if failures:
            raise UploadError(
                "OBS 上传未全部成功；成功=%d，失败=%d；%s"
                % (len(uploaded), len(failures), "；".join(failures))
            )
    finally:
        client.close()
    return uploaded


def upload_issue_json(
    source,
    run_date,
    bucket=DEFAULT_BUCKET,
    server=DEFAULT_SERVER,
    domain=DEFAULT_DOMAIN,
    access_key=None,
    secret_key=None,
    security_token=None,
    signature="auto",
    client_factory=None,
    header_factory=None,
    multipart_threshold=DEFAULT_MULTIPART_THRESHOLD,
    part_size=DEFAULT_PART_SIZE,
    multipart_tasks=DEFAULT_MULTIPART_TASKS,
):
    source, count = load_and_validate_json(source, run_date)
    keys = target_keys(run_date)
    uploaded = upload_objects(
        [(source, key, JSON_CONTENT_TYPE) for key in keys],
        bucket=bucket,
        server=server,
        domain=domain,
        access_key=access_key,
        secret_key=secret_key,
        security_token=security_token,
        signature=signature,
        client_factory=client_factory,
        header_factory=header_factory,
        multipart_threshold=multipart_threshold,
        part_size=part_size,
        multipart_tasks=multipart_tasks,
    )
    return count, uploaded


def upload_dashboard_bundle(
    source,
    report_source,
    run_date,
    excel_source=None,
    bucket=DEFAULT_BUCKET,
    server=DEFAULT_SERVER,
    domain=DEFAULT_DOMAIN,
    access_key=None,
    secret_key=None,
    security_token=None,
    signature="auto",
    client_factory=None,
    header_factory=None,
    multipart_threshold=DEFAULT_MULTIPART_THRESHOLD,
    part_size=DEFAULT_PART_SIZE,
    multipart_tasks=DEFAULT_MULTIPART_TASKS,
):
    """上传 Markdown 报告 →（可选 Excel）→ issue-tracking.json。

    每个对象都同时上传到 TARGET_TEMPLATES 的 A2_A3 与 A5 两个前缀；报告
    先上传可避免看板先看到 JSON 却打不开报告链接。
    """
    source, count = load_and_validate_json(source, run_date)
    report, report_type = validate_report_reference(source, report_source)
    _tmp_result = []
    for key in target_report_keys(run_date, report.name):
        _tmp_result.append((report, key, report_type))
    objects = _tmp_result
    if excel_source is not None:
        excel = validate_excel_source(excel_source, run_date)
        objects.extend(
            (excel, key, report_content_type(excel.name))
            for key in target_report_keys(run_date, excel.name)
        )
    objects.extend((source, key, JSON_CONTENT_TYPE) for key in target_keys(run_date))
    uploaded = upload_objects(
        objects,
        bucket=bucket,
        server=server,
        domain=domain,
        access_key=access_key,
        secret_key=secret_key,
        security_token=security_token,
        signature=signature,
        client_factory=client_factory,
        header_factory=header_factory,
        multipart_threshold=multipart_threshold,
        part_size=part_size,
        multipart_tasks=multipart_tasks,
    )
    return count, uploaded


def build_parser():
    parser = argparse.ArgumentParser(
        description="把日期化 Markdown（可选 Excel）和 issue-tracking.json "
        "同步到 A2_A3 和 A5"
    )
    parser.add_argument(
        "--file",
        default="output/issue-tracking.json",
        help="待上传 JSON，默认 output/issue-tracking.json",
    )
    report_group = parser.add_mutually_exclusive_group()
    report_group.add_argument(
        "--report-file", help="Markdown 报告；默认按 JSON 同目录和上传日期自动查找"
    )
    report_group.add_argument(
        "--json-only", action="store_true", help="显式只上传 JSON；每日正式交付不应使用"
    )
    parser.add_argument(
        "--excel-file", help="可选 Excel 附件；显式指定后与 Markdown 一同上传"
    )
    parser.add_argument(
        "--date",
        help="OBS 日期目录；默认从 JSON report_url 解析，历史空结果需显式指定 YYYYMMDD",
    )
    parser.add_argument("--bucket", default=DEFAULT_BUCKET)
    parser.add_argument("--server", default=DEFAULT_SERVER)
    parser.add_argument(
        "--domain", default=DEFAULT_DOMAIN, help="上传后展示的对象访问域名"
    )
    parser.add_argument(
        "--signature",
        choices=("auto", "obs", "v2", "v4"),
        default="auto",
        help="签名模式，默认自动协商",
    )
    parser.add_argument(
        "--single-put",
        action="store_true",
        help="禁用分段上传，直接 PutObject（用于网关不允许 multipart 时）",
    )
    parser.add_argument(
        "--dry-run", action="store_true", help="只校验 JSON 并显示目标，不连接 OBS"
    )
    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)
    try:
        if args.excel_file and args.json_only:
            raise ValueError("--excel-file 不能与 --json-only 同时使用")
        source, count = load_and_validate_json(args.file)
        run_date = (
            validate_date(args.date)
            if args.date
            else report_date_from_json(source) or current_dashboard_date()
        )
        source, count = load_and_validate_json(source, run_date)
        keys = target_keys(run_date)
        report = None
        excel = None
        if not args.json_only:
            report_source = args.report_file or default_report_source(source, run_date)
            report, _report_type = validate_report_reference(source, report_source)
            if args.excel_file:
                excel = validate_excel_source(args.excel_file, run_date)
        if args.dry_run:
            print("[ok] JSON 校验通过：%s（%d 条问题）" % (source, count))
            print("[ok] 上传日期：%s" % run_date)
            if report:
                print("[ok] 报告文件：%s" % report)
                for key in target_report_keys(run_date, report.name):
                    print("[dry-run] %s" % public_url(args.domain, key))
            if excel:
                print("[ok] Excel 附件：%s" % excel)
                for key in target_report_keys(run_date, excel.name):
                    print("[dry-run] %s" % public_url(args.domain, key))
            for key in keys:
                print("[dry-run] %s" % public_url(args.domain, key))
            return 0
        if report:
            count, uploaded = upload_dashboard_bundle(
                source,
                report,
                run_date,
                excel_source=excel,
                bucket=args.bucket,
                server=args.server,
                domain=args.domain,
                signature=args.signature,
                multipart_threshold=(
                    sys.maxsize if args.single_put else DEFAULT_MULTIPART_THRESHOLD
                ),
            )
        else:
            count, uploaded = upload_issue_json(
                source,
                run_date,
                bucket=args.bucket,
                server=args.server,
                domain=args.domain,
                signature=args.signature,
                multipart_threshold=(
                    sys.maxsize if args.single_put else DEFAULT_MULTIPART_THRESHOLD
                ),
            )
        print("[ok] 已上传 %d 条问题，共 %d 个对象" % (count, len(uploaded)))
        for url in uploaded:
            print("[ok] %s" % url)
        return 0
    except (FileNotFoundError, ValueError, RuntimeError) as exc:
        print("[error] %s" % exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())

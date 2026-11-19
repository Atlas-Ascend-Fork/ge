# 简介

`AnnotatedArgsContext`是当前`declare_launch_args`回调的声明式参数上下文，由[get_declare_launch_args_ctx](../get_declare_launch_args_ctx.md)返回。用户不能直接构造此对象。

该对象由native层提供，是仅在当前`declare_launch_args`回调内有效的借用视图。回调返回或抛出异常后，该对象会失效；其申请的`WorkspaceAddr`和创建的`AnnotatedKernelArgs`也会失效，再次访问会抛出`RuntimeError`。

除主逻辑流外，可以通过[request_attached_stream](request_attached_stream.md)按key申请逻辑辅流，并把返回的stream ID用于`AnnotatedKernelLaunchInfo`在辅流上提交launch。[add_launch](add_launch.md)支持通过`predecessors`声明此前成功提交的前驱launch，跨流依赖由GE自动生成Event同步任务，同流依赖不生成Event任务。

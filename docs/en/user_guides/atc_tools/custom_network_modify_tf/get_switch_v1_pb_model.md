# Obtaining the Switch\_v1.pb Network Model

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:18:58.065Z pushedAt=2026-10-28T03:58:08.899Z nl=2 -->

1. Copy the script in the following file to a .py file, for example, to the _Switch\_v1.py_ file.

    ```python
    import os
    import numpy as np
    import tensorflow as tf

    x = tf.compat.v1.placeholder(tf.float32, name='x')
    y = tf.compat.v1.placeholder(tf.float32, name='y')
    z = tf.compat.v1.placeholder(tf.float32, name='z')


    def then_branch(x, y, z):
        m = tf.square(x)
        return m + tf.multiply(y, z)

    def else_branch(x, y, z):
        m = tf.pow(x, y)
        return m - tf.div(y, z)

    # Entry for using control flow operators. After the script is executed, the corresponding V1 control flow operators are generated in the graph.
    def testDefun(x, y, z):
        return tf.cond(pred=x < y, true_fn=lambda: then_branch(x, y, z), false_fn=lambda: else_branch(x, y, z)), y

    def testCase(x, y, z):
        a, b = testDefun(x, y, z)
        return a + b * z


    with tf.compat.v1.Session() as sess:
        result = sess.run(testCase(x, y, z), feed_dict={x: 1., y: .6, z: .2})

        with tf.io.gfile.GFile('./Switch_v1.pb', 'wb') as f:
            f.write(sess.graph_def.SerializeToString())

    ```

2. Switch to the directory where the _Switch\_v1.py_ script is located, and run the following command to generate the Switch\_v1.pb network model:

    ```python
    python3.7.5 Switch_v1.py
    ```

    After the command is executed, the Switch\_v1.pb network model is generated in the current directory.

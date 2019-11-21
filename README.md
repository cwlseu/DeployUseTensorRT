# DeployUseTensorRT
Deploy awesome computer vision model use tensorrt

## 环境搭建

<!-- ![CUDA 8 FP16 and INT8 API and library support.](https://devblogs.nvidia.com/parallelforall/wp-content/uploads/2016/10/CUDA_mixed_precision_support-624x298.png) -->

The easiest way to benefit from mixed precision in your application is to take advantage of the support for FP16 and INT8 computation in NVIDIA GPU libraries. Key libraries from the NVIDIA SDK now support a variety of precisions for both computation and storage.

Table shows the current support for FP16 and INT8 in key CUDA libraries as well as in PTX assembly and CUDA C/C++ intrinsics.

|Feature| FP16x2|INT8/16 DP4A/DP2A|
|:----:|:-----:|:-----:|
|PTX instructions|CUDA 7.5| CUDA 8|
|CUDA C/C++ intrinsics|CUDA 7.5| CUDA 8|
|cuBLAS GEMM|CUDA 7.5| CUDA 8|
|cuFFT|CUDA 7.5| I/O via cuFFT callbacks|
|cuDNN|5.1| 6|
|TensorRT|v1| v2 Tech Preview|

PTX(parallel-thread-execution，并行线程执行) 预编译后GPU代码的一种形式，开发者可以通过编译选项 “-keep”选择输出PTX代码，
当然开发人员也可以直接编写PTX级代码。另外，PTX是独立于gpu架构的，因此可以重用相同的代码适用于不同的GPU架构。
具体可参考CUDA-PDF之[《PTX ISA reference document》](https://docs.nvidia.com/cuda/parallel-thread-execution/)

建议我们的CUDA 版本为CUDA 8.0以上, 显卡至少为`GeForce 1060`, 如果想支持Int8/DP4A等feature，还是需要`RTX 1080`或者`P40`。

### 个人的第三方deps目录

```
deps
├── cuda9.0
│   ├── bin
│   ├── include
│   ├── lib64
├── cudnn7.5
│   ├── include
│   ├── lib64
│   └── NVIDIA_SLA_cuDNN_Support.txt
└── tensorrt5.1.2
    ├── include
    └── lib
```

## Document for Reference

- [NVDLA官网](http://nvdla.org/)
- [NVIDIA blog: Production Deep Learning with NVIDIA GPU Inference Engine](https://devblogs.nvidia.com/production-deep-learning-nvidia-gpu-inference-engine/)
- [TensorRT 5.1的技术参数文档](https://developer.download.nvidia.cn/compute/machine-learning/tensorrt/docs/5.1/rc/TensorRT-Support-Matrix-Guide.pdf)
- [nvdla-sw-Runtime environment](http://nvdla.org/sw/runtime_environment.html)
- [Szymon Migacz, NVIDIA: 8-bit Inference with TensorRT](http://on-demand.gputechconf.com/gtc/2017/presentation/s7310-8-bit-inference-with-tensorrt.pdf)
- [INT8量化校准原理](https://arleyzhang.github.io/articles/923e2c40/)
- [Mixed-Precision Programming with CUDA 8](https://devblogs.nvidia.com/mixed-precision-programming-cuda-8/)
- [Tensorflow使用TensorRT高速推理](https://medium.com/tensorflow/high-performance-inference-with-tensorrt-integration-c4d78795fbfe)
- [Tensorflow使用TensorRT高速推理视频](https://on-demand.gputechconf.com/gtc/2019/video/_/S9431/)

### TensorRT跑的快

![@优化原理](https://img-blog.csdnimg.cn/20190907135522420.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3UwMTQzMDM2NDc=,size_16,color_FFFFFF,t_70)

![@原始网络](https://miro.medium.com/max/965/1*PyNcjHKZ8rQ48QCPsdQ9wA.png)
![@vertical fusion](https://miro.medium.com/max/951/1*bJts223Qo55toZ9AY60Ruw.png)
The above figures explain the vertical fusion optimization that TRT does. The Convolution (C), Bias(B) and Activation(R, ReLU in this case) are all collapsed into one single node (implementation wise this would mean a single CUDA kernel launch for C, B and R).

![@horizontal fusion](https://miro.medium.com/max/2000/0*UKwCx_lq-oHcLYkI.png)

There is also a horizontal fusion where if multiple nodes with same operation are feeding to multiple nodes then it is converted to one single node feeding multiple nodes. The three 1x1 CBRs are fused to one and their output is directed to appropriate nodes.
Other optimizations
Apart from the graph optimizations, TRT, through experiments and based on parameters like batch size, convolution kernel(filter) sizes, chooses efficient algorithms and kernels(CUDA kernels) for operations in network.

- DP4A(Dot Product of 4 8-bits Accumulated to a 32-bit)

TensorRT 进行优化的方式是 DP4A (Dot Product of 4 8-bits Accumulated to a 32-bit)，如下图：

![@DP4A原理过程](https://arleyzhang.github.io/images/TensorRT-5-int8-calibration.assets/DP4A.png)
这是PASCAL 系列GPU的硬件指令，INT8卷积就是使用这种方式进行的卷积计算。更多关于DP4A的信息可以参考[Mixed-Precision Programming with CUDA 8](https://devblogs.nvidia.com/mixed-precision-programming-cuda-8/)

INT8 vector dot products (DP4A) improve the efficiency of radio astronomy cross-correlation by a large factor compared to FP32 computation.

![@INT8 vector dot products (DP4A) improve the efficiency of radio astronomy cross-correlation by a large factor compared to FP32 computation](https://devblogs.nvidia.com/parallelforall/wp-content/uploads/2016/10/cross-correlation-efficiency-p40-624x453.png)

## TODO SCHEDULE

- [x] add test support
- [x] export a static lib
- [ ] plugin & extend layers
  - [ ] 设计plugin的管理机制,更新初始化流程
  - [ ] [interp](https://github.com/hszhao/PSPNet)
  - [ ] [ROIPooling](https://github.com/rbgirshick/caffe-fast-rcnn/tree/0dcd397b29507b8314e252e850518c5695efbb83)
  - [ ] [RPNProposal]()
  - [ ] [ChannelShuffle]()
  - [ ] [CTC]()
  - [ ] [SLLSTM]()

- [ ] int8 quantity inference
  - [ ] 矫正算法的设计
  - [ ] 量化数据集合的管理，这个可以和NNIE的量化数据统一起来管理
  - [ ] 与研究侧共同确定各个层量化的范围
  - [ ] 最后更新inference模式

- **model load sample**
  模型初始化当前包括通过parser初始化和通过模型流初始化的方式。通过parser初始化过程相比较来说比较慢，因为包含parser过程
  - [x] caffe model
  - [x] gie model


## Model Zoo

- [SqueezeNet](https://github.com/DeepScale/SqueezeNet)
- [Sequence to Sequence -- Video to Text]()
- [OpenPose](https://github.com/CMU-Perceptual-Computing-Lab/openpose/blob/master/models/getModels.sh)
   https://github.com/CMU-Perceptual-Computing-Lab/openpose
- wget https://s3.amazonaws.com/download.onnx/models/opset_3/resnet50.tar.gz (Link source: https://github.com/onnx/models/tree/master/resnet50)
## 附录

### Init.CaffeModel
```
[I] Output "prob": 1000x1x1
[I] [TRT] Applying generic optimizations to the graph for inference.
[I] [TRT] Original: 141 layers
[I] [TRT] After dead-layer removal: 141 layers
[I] [TRT] After scale fusion: 141 layers
[I] [TRT] Fusing conv1/7x7_s2 with conv1/relu_7x7
[I] [TRT] Fusing conv2/3x3_reduce with conv2/relu_3x3_reduce
。。。
[I] [TRT] Fusing inception_5b/pool_proj with inception_5b/relu_pool_proj
[I] [TRT] After vertical fusions: 84 layers
[I] [TRT] After swap: 84 layers
[I] [TRT] After final dead-layer removal: 84 layers
[I] [TRT] Merging layers: inception_3a/1x1 + inception_3a/relu_1x1 || inception_3a/3x3_reduce + inception_3a/relu_3x3_reduce || inception_3a/5x5_reduce + inception_3a/relu_5x5_reduce
[I] [TRT] Merging layers: inception_3b/1x1 + inception_3b/relu_1x1 || inception_3b/3x3_reduce + inception_3b/relu_3x3_reduce || inception_3b/5x5_reduce + inception_3b/relu_5x5_reduce
。。。
[I] [TRT] Merging layers: inception_5b/1x1 + inception_5b/relu_1x1 || inception_5b/3x3_reduce + inception_5b/relu_3x3_reduce || inception_5b/5x5_reduce + inception_5b/relu_5x5_reduce
[I] [TRT] After tensor merging: 66 layers
[I] [TRT] Eliminating contatenation inception_3a/output
[I] [TRT] Generating copy for inception_3a/1x1 + inception_3a/relu_1x1 || inception_3a/3x3_reduce + inception_3a/relu_3x3_reduce || inception_3a/5x5_reduce + inception_3a/relu_5x5_reduce to inception_3a/output
[I] [TRT] Retargeting inception_3a/3x3 to inception_3a/output
[I] [TRT] Retargeting inception_3a/5x5 to inception_3a/output
[I] [TRT] Retargeting inception_3a/pool_proj to inception_3a/output
[I] [TRT] Eliminating contatenation inception_3b/output
[I] [TRT] Generating copy for inception_3b/1x1 + inception_3b/relu_1x1 || inception_3b/3x3_reduce + inception_3b/relu_3x3_reduce || inception_3b/5x5_reduce + inception_3b/relu_5x5_reduce to inception_3b/output
[I] [TRT] Retargeting inception_3b/3x3 to inception_3b/output
[I] [TRT] Retargeting inception_3b/5x5 to inception_3b/output
[I] [TRT] Retargeting inception_3b/pool_proj to inception_3b/output
[I] [TRT] Eliminating contatenation inception_4a/output
[I] [TRT] Generating copy for inception_4a/1x1 + inception_4a/relu_1x1 || inception_4a/3x3_reduce + inception_4a/relu_3x3_reduce || inception_4a/5x5_reduce + inception_4a/relu_5x5_reduce to inception_4a/output
[I] [TRT] Retargeting inception_4a/3x3 to inception_4a/output
[I] [TRT] Retargeting inception_4a/5x5 to inception_4a/output
[I] [TRT] Retargeting inception_4a/pool_proj to inception_4a/output
[I] [TRT] Eliminating contatenation inception_4b/output
[I] [TRT] Generating copy for inception_4b/1x1 + inception_4b/relu_1x1 || inception_4b/3x3_reduce + inception_4b/relu_3x3_reduce || inception_4b/5x5_reduce + inception_4b/relu_5x5_reduce to inception_4b/output
[I] [TRT] Retargeting inception_4b/3x3 to inception_4b/output
[I] [TRT] Retargeting inception_4b/5x5 to inception_4b/output
[I] [TRT] Retargeting inception_4b/pool_proj to inception_4b/output
[I] [TRT] Eliminating contatenation inception_4c/output
[I] [TRT] Generating copy for inception_4c/1x1 + inception_4c/relu_1x1 || inception_4c/3x3_reduce + inception_4c/relu_3x3_reduce || inception_4c/5x5_reduce + inception_4c/relu_5x5_reduce to inception_4c/output
[I] [TRT] Retargeting inception_4c/3x3 to inception_4c/output
[I] [TRT] Retargeting inception_4c/5x5 to inception_4c/output
[I] [TRT] Retargeting inception_4c/pool_proj to inception_4c/output
[I] [TRT] Eliminating contatenation inception_4d/output
[I] [TRT] Generating copy for inception_4d/1x1 + inception_4d/relu_1x1 || inception_4d/3x3_reduce + inception_4d/relu_3x3_reduce || inception_4d/5x5_reduce + inception_4d/relu_5x5_reduce to inception_4d/output
[I] [TRT] Retargeting inception_4d/3x3 to inception_4d/output
[I] [TRT] Retargeting inception_4d/5x5 to inception_4d/output
[I] [TRT] Retargeting inception_4d/pool_proj to inception_4d/output
[I] [TRT] Eliminating contatenation inception_4e/output
[I] [TRT] Generating copy for inception_4e/1x1 + inception_4e/relu_1x1 || inception_4e/3x3_reduce + inception_4e/relu_3x3_reduce || inception_4e/5x5_reduce + inception_4e/relu_5x5_reduce to inception_4e/output
[I] [TRT] Retargeting inception_4e/3x3 to inception_4e/output
[I] [TRT] Retargeting inception_4e/5x5 to inception_4e/output
[I] [TRT] Retargeting inception_4e/pool_proj to inception_4e/output
[I] [TRT] Eliminating contatenation inception_5a/output
[I] [TRT] Generating copy for inception_5a/1x1 + inception_5a/relu_1x1 || inception_5a/3x3_reduce + inception_5a/relu_3x3_reduce || inception_5a/5x5_reduce + inception_5a/relu_5x5_reduce to inception_5a/output
[I] [TRT] Retargeting inception_5a/3x3 to inception_5a/output
[I] [TRT] Retargeting inception_5a/5x5 to inception_5a/output
[I] [TRT] Retargeting inception_5a/pool_proj to inception_5a/output
[I] [TRT] Eliminating contatenation inception_5b/output
[I] [TRT] Generating copy for inception_5b/1x1 + inception_5b/relu_1x1 || inception_5b/3x3_reduce + inception_5b/relu_3x3_reduce || inception_5b/5x5_reduce + inception_5b/relu_5x5_reduce to inception_5b/output
[I] [TRT] Retargeting inception_5b/3x3 to inception_5b/output
[I] [TRT] Retargeting inception_5b/5x5 to inception_5b/output
[I] [TRT] Retargeting inception_5b/pool_proj to inception_5b/output
[I] [TRT] After concat removal: 66 layers
[I] [TRT] Graph construction and optimization completed in 0.00874238 seconds.
[I] [TRT] 
[I] [TRT] --------------- Timing conv1/7x7_s2 + conv1/relu_7x7(3)
[I] [TRT] Tactic 0 time 0.370688
[I] [TRT] 
[I] [TRT] --------------- Timing conv1/7x7_s2 + conv1/relu_7x7(14)
[I] [TRT] Tactic 3146172331490511787 time 0.694752
[I] [TRT] Tactic 3528302785056538033 time 0.429056
[I] [TRT] Tactic -6618588952828687390 time 0.419296
[I] [TRT] Tactic -6362554771847758902 time 0.371168
[I] [TRT] Tactic -2701242286872672544 time 0.685056
[I] [TRT] Tactic -675401754313066228 time 0.365568
[I] [TRT] 
。。。
[I] [TRT] Tactic 5 time 0.032192
[I] [TRT] 
[I] [TRT] --------------- Timing loss3/classifier(15)
[I] [TRT] Tactic 2624962759642542471 time 0.07424
[I] [TRT] Tactic 6241535668063793554 time 0.094688
[I] [TRT] Tactic 8292480392881939394 time 0.074752
[I] [TRT] Tactic 8436800165353340181 time 0.059936
[I] [TRT] Tactic -7597689592892725774 time 0.09216
[I] [TRT] --------------- Chose 6 (5)
[I] [TRT] 
[I] [TRT] --------------- Timing prob(11)
[I] [TRT] Tactic 0 is the only option, timing skipped
[I] [TRT] Formats and tactics selection completed in 10.0197 seconds.
[I] [TRT] After reformat layers: 66 layers
[I] [TRT] Block size 1073741824
[I] [TRT] Block size 12845056
[I] [TRT] Block size 9633792
[I] [TRT] Block size 3211264
[I] [TRT] Block size 3211264
[I] [TRT] Total Activation Memory: 1102643200
[I] [TRT] Detected 1 input and 1 output network tensors.
[I] [TRT] Data initialization and engine generation completed in 0.0458818 seconds.
loadmodel time: 10322 ms
infer time: 8.20 ms
```

### Init.GIEModel
```
[I] [TRT] Glob Size is 40869280 bytes.
[I] [TRT] Added linear block of size 3211264
[I] [TRT] Added linear block of size 2408448
[I] [TRT] Added linear block of size 802816
[I] [TRT] Added linear block of size 802816
[I] [TRT] Deserialize required 13227 microseconds.
[I] googlenet_gie.bin has been successfully loaded.
loadmodel time: 36 ms
infer time: 2.80 ms
```

###  IPlugin接口中需要被重载的函数

1) 确定输出：一个是通过`int getNbOutput()`得到output输出的数目，即用户所定义的一层有几个输出。另一个是通过`Dims getOutputDimensions (int index, const Dims* inputs, int nbInputDims)` 得到整个输出的维度信息，大家可能不一定遇到有多个输出，一般来讲只有一个输出，但是大家在做检测网络的时候可能会遇到多个输出，一个输出是实际的检测目标是什么，另一个输出是目标的数目，可能的过个输出需要设定Dimension的大小。

2) 层配置：通过`void configure()` 实现构建推断（Inference） engine时模型中相应的参数大小等配置，configure()只是在构建的时候调用，这个阶段确定的东西是在运行时作为插件参数来存储、序列化/反序列化的。

3) 资源管理：通过`void Initialize()`来进行资源的初始化，`void terminate()`来销毁资源，甚至中间可能会有一些临时变量，也可以使用这两个函数进行初始化或销毁。需要注意的是，void Initialize()和void terminate()是在整个运行时都被调用的，并不是做完一次推断（Inference）就去调用terminate。相当于在线的一个服务，服务起的时候会调用void Initialize()，而服务止的时候调用void terminate()，但是服务会进进出出很多sample去做推断（Inference）。

4) 执行(Execution)：`void enqueue()`来定义用户层的操作

5) 序列化和反序列化：这个过程是将层的参数写入到二进制文件中，需要定义一些序列化的方法。通过`size_t getSerializationSize()`获得序列大小，通过void serialize()将层的参数序列化到缓存中，通过PluginSample()从缓存中将层参数反序列化。需要注意的是，TensorRT没有单独的反序列化的API，因为不需要，在实习构造函数的时候就完成了反序列化的过程

6) 从Caffe Parser添加Plugin：首先通过`Parsernvinfer1::IPlugin* createPlugin()`实现nvcaffeparser1::IPlugin 接口，然后传递工厂实例到`ICaffeParser::parse()`，Caffe的Parser才能识别

7) 运行时创建插件：通过`IPlugin* createPlugin()`实现nvinfer1::IPlugin接口，传递工厂实例到`IInferRuntime::deserializeCudaEngine()`
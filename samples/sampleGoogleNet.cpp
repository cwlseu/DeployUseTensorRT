﻿//!
//! CaffeGoogleNet.cpp
//! This file contains the implementation of the GoogleNet sample. It creates the network using
//! the GoogleNet caffe model.
//! It can be run with the following command line:
//! Command: ./sample_googlenet [-h or --help] [-d=/path/to/data/dir or --datadir=/path/to/data/dir]
//!
/** GoogleNet include layer type：
    Convolution
    Dropout
    ReLU
    LRN
    Pooling
    InnerProduct
    Softmax
    Input
    Concat
all the layer have supported in TensorRT libs
**/
#include "common/argsParser.h"
#include "common/buffers.h"
#include "common/logger.h"
#include "common/common.h"

#include "NvCaffeParser.h"
#include "NvInfer.h"
#include <cuda_runtime_api.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include "BaseModel.h"
const std::string gSampleName = "TensorRT.sample_googlenet";

//!
//! \brief  The CaffeGoogleNet class implements the GoogleNet sample
//!
//! \details It creates the network using a caffe model
//!
class CaffeGoogleNet : IBaseModel
{
    template <typename T>
    using SampleUniquePtr = std::unique_ptr<T, dtrCommon::DtrInferDeleter>;

public:
    CaffeGoogleNet(const dtrCommon::CaffeNNParams& params)
        : mParams(params)
    {
    }

    //!
    //! \brief Function builds the network engine
    //!
    bool build();

    //!
    //! \brief This function runs the TensorRT inference engine for this sample
    //!
    std::vector<DataBlob32f> infer(const std::vector<DataBlob32f>& input_blobs);

    //!
    //! \brief This function can be used to clean up any state created in the sample class
    //!
    bool teardown();

    dtrCommon::CaffeNNParams mParams;

private:
    std::shared_ptr<nvinfer1::ICudaEngine> mEngine = nullptr; //!< The TensorRT engine used to run the network

    //!
    //! \brief This function parses a Caffe model for GoogleNet and creates a TensorRT network
    //!
    void constructNetwork(SampleUniquePtr<nvinfer1::IBuilder>& builder, SampleUniquePtr<nvinfer1::INetworkDefinition>& network, SampleUniquePtr<nvcaffeparser1::ICaffeParser>& parser);
};

//!
//! \brief This function creates the network, configures the builder and creates the network engine
//!
//! \details This function creates the GoogleNet network by parsing the caffe model and builds
//!          the engine that will be used to run GoogleNet (mEngine)
//!
//! \return Returns true if the engine was created successfully and false otherwise
//!
bool CaffeGoogleNet::build()
{
    auto builder = SampleUniquePtr<nvinfer1::IBuilder>(nvinfer1::createInferBuilder(gLogger.getTRTLogger()));
    if (!builder)
        return false;

    auto network = SampleUniquePtr<nvinfer1::INetworkDefinition>(builder->createNetwork());
    if (!network)
        return false;

    auto parser = SampleUniquePtr<nvcaffeparser1::ICaffeParser>(nvcaffeparser1::createCaffeParser());
    if (!parser)
        return false;

    constructNetwork(builder, network, parser);

    mEngine = std::shared_ptr<nvinfer1::ICudaEngine>(builder->buildCudaEngine(*network), dtrCommon::DtrInferDeleter());
    if (!mEngine)
        return false;

    return true;
}

//!
//! \brief This function uses a caffe parser to create the googlenet Network and marks the
//!        output layers
//!
//! \param network Pointer to the network that will be populated with the googlenet network
//!
//! \param builder Pointer to the engine builder
//!
void CaffeGoogleNet::constructNetwork(SampleUniquePtr<nvinfer1::IBuilder>& builder, SampleUniquePtr<nvinfer1::INetworkDefinition>& network, SampleUniquePtr<nvcaffeparser1::ICaffeParser>& parser)
{
    const nvcaffeparser1::IBlobNameToTensor* blobNameToTensor = parser->parse(
        locateFile(mParams.prototxtFileName, mParams.dataDirs).c_str(),
        locateFile(mParams.weightsFileName, mParams.dataDirs).c_str(),
        *network,
        nvinfer1::DataType::kFLOAT);

    for (auto& s : mParams.outputTensorNames)
        network->markOutput(*blobNameToTensor->find(s.c_str()));

    builder->setMaxBatchSize(mParams.batchSize);
    builder->setMaxWorkspaceSize(16_MB);
    dtrCommon::enableDLA(builder.get(), mParams.useDLACore);
}

//!
//! \brief This function runs the TensorRT inference engine for this sample
//!
//! \details This function is the main execution function of the sample. It allocates the buffer,
//!          sets inputs and executes the engine.
//!
std::vector<DataBlob32f> CaffeGoogleNet::infer(const std::vector<DataBlob32f>& input_blobs)
{
    // Create RAII buffer manager object
    dtrCommon::BufferManager buffers(mEngine, mParams.batchSize);

    auto context = SampleUniquePtr<nvinfer1::IExecutionContext>(mEngine->createExecutionContext());
    if (!context)
        return {};

    // Fetch host buffers and set host input buffers to all zeros
    for (auto& input : mParams.inputTensorNames)
        memset(buffers.getHostBuffer(input), 0, buffers.size(input));

    // Memcpy from host input buffers to device input buffers
    buffers.copyInputToDevice();

    bool status = context->execute(mParams.batchSize, buffers.getDeviceBindings().data());
    if (!status)
        return {};

    // Memcpy from device output buffers to host output buffers
    buffers.copyOutputToHost();

    return {};
}

//!
//! \brief This function can be used to clean up any state created in the sample class
//!
bool CaffeGoogleNet::teardown()
{
    //! Clean up the libprotobuf files as the parsing is complete
    //! \note It is not safe to use any other part of the protocol buffers library after
    //! ShutdownProtobufLibrary() has been called.
    nvcaffeparser1::shutdownProtobufLibrary();
    return true;
}

//!
//! \brief This function initializes members of the params struct using the command line args
//!
dtrCommon::CaffeNNParams initializeNNParams(const dtrCommon::Args& args)
{
    dtrCommon::CaffeNNParams params;
    if (args.dataDirs.size() != 0) //!< Use the data directory provided by the user
        params.dataDirs = args.dataDirs;
    else //!< Use default directories if user hasn't provided directory paths
    {
        params.dataDirs.push_back("data/googlenet/");
    }
    params.prototxtFileName = "googlenet.prototxt";
    params.weightsFileName = "googlenet.caffemodel";
    params.inputTensorNames.push_back("data");
    params.batchSize = 4;
    params.outputTensorNames.push_back("prob");
    params.useDLACore = args.useDLACore;

    return params;
}
//!
//! \brief This function prints the help information for running this sample
//!
void printHelpInfo()
{
    std::cout << "Usage: ./sample_googlenet [-h or --help] [-d or --datadir=<path to data directory>] [--useDLACore=<int>]\n";
    std::cout << "--help          Display help information\n";
    std::cout << "--datadir       Specify path to a data directory, overriding the default. This option can be used multiple times to add multiple directories. If no data directories are given, the default is to use data/samples/googlenet/ and data/googlenet/" << std::endl;
    std::cout << "--useDLACore=N  Specify a DLA engine for layers that support DLA. Value can range from 0 to n-1, where n is the number of DLA engines on the platform." << std::endl;
}

int main(int argc, char** argv)
{
    setReportableSeverity(Logger::Severity::kINFO);
    initLibNvInferPlugins(&gLogger.getTRTLogger(), "");
    dtrCommon::Args args;
    bool argsOK = dtrCommon::parseArgs(args, argc, argv);
    if (args.help) {
        printHelpInfo();
        return EXIT_SUCCESS;
    }
    if (!argsOK) {
        LOG_ERROR(gLogger) << "Invalid arguments" << std::endl;
        printHelpInfo();
        return EXIT_FAILURE;
    }

    auto sampleTest = gLogger.defineTest(gSampleName, argc, const_cast<const char**>(argv));

    gLogger.reportTestStart(sampleTest);

    dtrCommon::CaffeNNParams params = initializeNNParams(args);
    CaffeGoogleNet sample(params);

    LOG_INFO(gLogger) << "Building and running a GPU inference engine for GoogleNet" << std::endl;

    if (!sample.build()) {
        return gLogger.reportFail(sampleTest);
    }
    sample.infer({});
    if (!sample.teardown()) {
        return gLogger.reportFail(sampleTest);
    }

    LOG_INFO(gLogger) << "Ran " << argv[0] << " with: " << std::endl;

    std::stringstream ss;

    ss << "Input(s): ";
    for (auto& input : sample.mParams.inputTensorNames)
        ss << input << " ";
    LOG_INFO(gLogger) << ss.str() << std::endl;

    ss.str(std::string());

    ss << "Output(s): ";
    for (auto& output : sample.mParams.outputTensorNames)
        ss << output << " ";
    LOG_INFO(gLogger) << ss.str() << std::endl;

    return gLogger.reportPass(sampleTest);
}

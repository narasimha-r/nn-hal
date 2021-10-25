#include <Log_Softmax.hpp>
#undef LOG_TAG
#define LOG_TAG "Log_Softmax"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace nnhal {

Log_Softmax::Log_Softmax(int operationIndex) : OperationsBase(operationIndex) {
    mDefaultOutputIndex = sModelInfo->getOperationOutput(mNnapiOperationIndex, 0);
}

bool Log_Softmax::validate() {
    ALOGV("%s PASSED", __func__);
    return true;
}

std::shared_ptr<ngraph::Node> Log_Softmax::createNode() {
    // Creating input nodes
    std::shared_ptr<ngraph::Node> input, outputNode;

    input = getInputNode(0);

    std::shared_ptr<ngraph::Node> betaNode;

    if (checkInputOperandType(0, (int32_t)OperandType::TENSOR_FLOAT16)) {
        auto beta = sModelInfo->ParseOperationInput<_Float16>(mNnapiOperationIndex, 1);
        betaNode = createConstNode(ngraph::element::f16, {}, convertToVector(beta));
    } else {
        auto beta = sModelInfo->ParseOperationInput<float>(mNnapiOperationIndex, 1);
        betaNode = createConstNode(ngraph::element::f32, {}, convertToVector(beta));
    }
    int axis = -1;
    const auto& inputsSize = sModelInfo->getOperationInputsSize(mNnapiOperationIndex);
    if (inputsSize == 3) axis = sModelInfo->ParseOperationInput<int>(mNnapiOperationIndex, 2);

    const auto axisNode = createConstNode(ngraph::element::i32, {1}, convertToVector(axis));

    // logits * beta
    auto mul = std::make_shared<ngraph::opset3::Multiply>(input, betaNode);
    // exp(logits * beta)
    auto exp = std::make_shared<ngraph::opset3::Exp>(mul);
    // reduce_sum(exp(logits * beta), axis)
    auto sum = std::make_shared<ngraph::opset3::ReduceSum>(exp, axisNode, true);
    // log(reduce_sum(exp(logits * beta), axis))
    auto log = std::make_shared<ngraph::opset3::Log>(sum);
    // logits * beta - log(reduce_sum(exp(logits * beta), axis))
    outputNode = std::make_shared<ngraph::opset3::Subtract>(mul, log);

    return outputNode;
}

}  // namespace nnhal
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

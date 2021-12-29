#include <torch/torch.h>
#include <torch/script.h>

int main() {
    torch::jit::script::Module module;
    module = torch::jit::load("foo.pt");
    // Create a vector of inputs.
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(torch::ones({1, 20}));

    // Execute the model and turn its output into a tensor.
    at::Tensor output = module.forward(inputs).toTensor();
    std::cout << output << "\n";
    return 0;
}

#include <torch/torch.h>
#include <torch/script.h>

int main() {
    torch::jit::script::Module module;
    module = torch::jit::load("model.pt");
    // Create a vector of inputs.
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(torch::ones({10, 20}));

    // Execute the model and turn its output into a tensor.
    at::Tensor output1 = module.forward(inputs).toTensor();
    std::cout << output1 << "\n";


    torch::jit::IValue input_a = torch::ones({10, 20});
    torch::jit::IValue input_b = torch::ones({10, 20});
    at::Tensor output2 = module.run_method("another_forward", input_a, input_b).toTensor();
    std::cout << output2 << "\n";

    return 0;
}

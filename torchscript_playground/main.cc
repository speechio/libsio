#include <torch/torch.h>
#include <torch/script.h>

int main() {

    //torch::Tensor frame0 = torch::rand({80});
    //torch::Tensor frame1 = torch::rand({80});

    //std::string model_path = "/home/speechio/work/git/speechio/exp/AISHELL-1/final.pt";
    //torch::jit::script::Module model = torch::jit::load(model_path);

    //torch::jit::IValue o1 = model.run_method("is_bidirectional_decoder");
    //std::cout << "bidirectional: " << o1.toBool() << "\n";

    //torch::jit::IValue o2 = model.run_method("subsampling_rate");
    //std::cout << "subsampling_rate: " << o2.toInt() << "\n";

    torch::jit::script::Module model;
    model = torch::jit::load("model.pt");
    model.eval();

    // Create a vector of inputs.
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(torch::ones({10, 20}));

    // Execute the model and turn its output into a tensor.
    torch::Tensor output1 = model.forward(inputs).toTensor();
    std::cout << output1 << "\n";

    // try explicitly exported method rather than "forward"
    // forward accept a vector of input(aka batch)
    // other methods follow their own prototype
    torch::Tensor input_a = torch::ones({10, 20});
    torch::Tensor input_b = torch::ones({10, 20});
    torch::Tensor output2 = model.run_method("another_forward", input_a, input_b).toTensor();
    std::cout << output2 << "\n";

    return 0;
}


import torch

class MyModule(torch.nn.Module):
    def __init__(self, N, M):
        super(MyModule, self).__init__()
        self.weight = torch.nn.Parameter(torch.ones(N, M))

    def forward(self, input):
        if input.sum() > 0:
          output = self.weight + input
        else:
          output = -self.weight
        return output

    @torch.jit.export
    def another_forward(self, input_a, input_b):
        if (input_a + input_b).sum() > 0:
          output = self.weight - 1
        else:
          output = -self.weight
        return output

my_module = MyModule(10,20)
sm = torch.jit.script(my_module)

print(sm.graph)
print(sm.code)

sm.save("model.pt")


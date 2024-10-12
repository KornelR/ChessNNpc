import sys
import torch
import atexit
import torch.nn as nn
import torch.optim as optim

class Model(nn.Module):
    def __init__(self, hidden_layer_sizes, activation_function='relu'):
        super(Model, self).__init__()
        
        # Input layer
        self.layers = nn.ModuleList()
        self.layers.append(nn.Linear(387, hidden_layer_sizes[0]))  # Input to first hidden layer
        
         # Hidden layers
        for i in range(1, len(hidden_layer_sizes)):
            self.layers.append(nn.Linear(hidden_layer_sizes[i-1], hidden_layer_sizes[i]))
        
        # Output layer
        self.layers.append(nn.Linear(hidden_layer_sizes[-1], 128))
        
        # Choose activation function
        if activation_function == 'relu':
            self.activation = nn.ReLU()
        elif activation_function == 'sigmoid':
            self.activation = nn.Sigmoid()

    def forward(self, x):
        for layer in self.layers[:-1]:  # Iterate through hidden layers
            x = self.activation(layer(x))
        x = self.layers[-1](x)  # Output layer (no activation function here)
        return x

hidden_layer_sizes = [1024, 2048, 4096, 4096, 4096, 4096, 2048, 1024, 512]

model = Model(hidden_layer_sizes, activation_function='relu')

torch.save(model.state_dict(), 'weights.pth')



def custom_loss(output, target):
    # Separate output into two parts
    output_1 = output[:, :64]  # First 64 outputs
    output_2 = output[:, 64:]  # Second 64 outputs
    
    # Calculate the maximum for each part
    max_pred_1 = torch.max(output_1, dim=1)[0]  # Maximum of first part
    max_pred_2 = torch.max(output_2, dim=1)[0]  # Maximum of second part
    
    # Calculate the loss
    loss_1 = nn.MSELoss()(max_pred_1, target[:, 0])  # Target for first part
    loss_2 = nn.MSELoss()(max_pred_2, target[:, 1])  # Target for second part
    
    return loss_1 + loss_2  # Combine the losses

def process_input():  
    model.eval()
    input_array = []

    for i in range(2, 389):
        value = float(sys.argv[i])
        input_array.append(value)
        
    input_tensor = torch.tensor(input_array, dtype=torch.float32)
    output = model(input_tensor)
    output += 2
    output *= 100

    returnString = []

    for i in range(0, 128):
        value = str(output[i])
        value = value.replace('tensor(', '')
        value = value.replace(', grad_fn=<SelectBackward0>)','')      
        returnString += value
        returnString += "N"
              
    print(*returnString)
   
def backpropagation():  
    input_array = []
    
    for i in range(2, 389):
        value = float(sys.argv[i])
        input_array.append(value)
    
    model.train()
    input_tensor = torch.tensor(input_array, dtype=torch.float32)

    target_array = []
    for j in range(390, 517):
        target = float(sys.argv[i])
        target_array.append(target)
        
    criterion = nn.MSELoss()
    optimizer = optim.SGD(model.parameters(), lr=0.01)
    
    output = model(input_tensor)
    loss = criterion(output, target_array)
    optimizer.zero_grad()
    loss.backward()
    optimizer.step()

    
def exit_handler():
    state_dict = torch.load('weights.pth', weights_only=False)
    model.load_state_dict(state_dict)  
    model.eval()

atexit.register(exit_handler)

if __name__ == "__main__":
    


    mode = sys.argv[1]
    
    if mode == "process_input":
        process_input()
    if mode == "backpropagation":
        backpropagation()



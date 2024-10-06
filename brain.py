import sys
import torch
import torch.nn as nn
import torch.optim as optim

class ChessNN(nn.Module):
    def __init__(self, hidden_layer_sizes, activation_function='relu'):
        super(ChessNN, self).__init__()
        
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


def main():
    name = sys.argv[1]  
    name2 = sys.argv[2] 
    print(f"Hello, {name}!")
    print(f"Hello, {name2}!")
    


if __name__ == "__main__":
    main()
    
    hidden_layer_sizes = [1024, 2048, 4096, 4096, 4096, 4096, 2048, 1024, 512]
    
    model = ChessNN(hidden_layer_sizes, activation_function='relu')
    
    torch.save(model.state_dict(), 'weights.pth')

     # Define optimizer
    optimizer = optim.Adam(model.parameters(), lr=0.001)




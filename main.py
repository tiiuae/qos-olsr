# This is a sample Python script.
import keras.metrics
# Press Shift+F10 to execute it or replace it with your code.
# Press Double Shift to search everywhere for classes, files, tool windows, actions, and settings.


import os
import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.optimizers import Adam
from collections import deque
import random
import matplotlib.pyplot as plt
plt.switch_backend('TkAgg')  # or any other supported backend


# Define the environment and agent
class DisconnectionEnv:
    def __init__(self, data):
        self.data = data
        self.current_step = 0
        self.max_steps = len(data) - 1

    def reset(self):
        self.current_step = 0
        return self._get_observation()

    def _get_observation(self):
        print("at _get_observation")
        arr = np.array(self.data.iloc[self.current_step])
        arr = arr[:-1]
        return arr

    def step(self, action):
        self.current_step += 1
        done = self.current_step == self.max_steps
        reward = 1 if self.data['Connected'].iloc[self.current_step - 1] >= 0.5 else -1

        if done:
            next_state = None
        else:
            next_state = self._get_observation()

        return next_state, reward, done

# Define the DQN agent
class DQNAgent:
    def __init__(self, state_size, action_size):
        self.state_size = state_size
        self.action_size = action_size
        self.memory = deque(maxlen=2000)
        self.gamma = 0.95  # discount factor
        self.epsilon = 1.0  # exploration-exploitation trade-off
        self.epsilon_decay = 0.995
        self.epsilon_min = 0.01
        self.learning_rate = 0.005
        self.model = self._build_model()

    def _build_model(self):
        model = Sequential()
        model.add(Dense(24, input_dim=self.state_size, activation='relu'))
        model.add(Dense(24, activation='relu'))
        model.add(Dense(self.action_size, activation='sigmoid'))  # Use sigmoid activation for binary classification
        model.compile(loss='mse', optimizer=Adam(lr=self.learning_rate))
        return model

    def remember(self, state, action, reward, next_state, done):
        self.memory.append((state, action, reward, next_state, done))

    def act(self, state):
        if np.random.rand() <= self.epsilon:
            return np.random.choice(self.action_size)
        q_values = self.model.predict(state.astype(np.float32))
        return np.argmax(q_values[0])

    def replay(self, batch_size):
        minibatch = random.sample(self.memory, batch_size)

        states = []
        targets = []

        for state, action, reward, next_state, done in minibatch:
            target = reward
            if not done:
                print("we are inside replay>for>if not done")
                next_state = np.reshape(next_state, [1, self.state_size])
                target = reward + self.gamma * np.amax(self.model.predict(next_state.astype(np.float32))[0])

            state = np.reshape(state, [1, self.state_size])
            targets.append(target)
            states.append(state)

        states = np.vstack(states).astype(np.float32)
        targets = np.vstack(targets).astype(np.float32)

        self.model.fit(states, targets, epochs=1, verbose=1)

        if self.epsilon > self.epsilon_min:
            self.epsilon *= self.epsilon_decay

    def save_model_weights(self, filename):
        self.model.save_weights(filename)

    def load_model_weights(self, filename):
        self.model.load_weights(filename)


def measure_success_rate(env, agent, episodes):
    successful_episodes = 0

    for _ in range(episodes):
        state = env.reset()
        state = np.reshape(state, [1, agent.state_size])
        done = False

        # Use the agent's predict function to get the predicted value
        predicted_value = agent.model.predict(state)[0]

        # Define a threshold for success, adjust as needed
        threshold = 0.5

        # Check if the predicted value is above the threshold
        if predicted_value > threshold:
            successful_episodes += 1

    success_rate = successful_episodes / episodes
    return success_rate


# Function to recursively get all CSV files in a directory
def get_csv_files(directory):
    csv_files = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(".csv"):
                csv_files.append(os.path.join(root, file))
    return csv_files


def main1():
    # Load dataset
    # df0 = pd.read_csv('/home/ayaalyousef/workspace/QoS-OLSR 4.0 Files/Pngdata/NoiseDistanceGroundF/0.csv')
    # df1 = pd.read_csv('/home/ayaalyousef/workspace/QoS-OLSR 4.0 Files/Pngdata/NoiseDistanceGroundF/1.csv')
    # df2 = pd.read_csv('/home/ayaalyousef/workspace/QoS-OLSR 4.0 Files/Pngdata/NoiseDistanceGroundF/2.csv')

    #***
    data_folder = '/home/ayaalyousef/workspace/QoS-OLSR 4.0 Files/Pngdata/'
    # Get all CSV files
    csv_files = get_csv_files(data_folder)

    # Read each CSV file
    dfs = []
    for csv_file in csv_files:
        df = pd.read_csv(csv_file)
        dfs.append(df)

    # Concatenate all DataFrames into one
    # combined_df = pd.concat(dfs, ignore_index=True)

    #***

    # df = pd.concat([df0, df1, df2], axis=0, ignore_index=True)

    # Concatenate all DataFrames into one
    df = pd.concat(dfs, axis=0, ignore_index=True)
    df = df.iloc[:, :-1]  # Remove the last empty column
    df = df.drop(["systemTime", "ipAddress"], axis='columns')
    print(f"df.shape = {df.shape}")
    # Feature scaling
    scaler = StandardScaler()
    df[['linkCost', 'linkQuality', 'neighborLinkQuality', 'RSSI value', 'AVG RSSI value']] = scaler.fit_transform(
        df[['linkCost', 'linkQuality', 'neighborLinkQuality', 'RSSI value', 'AVG RSSI value']])

    # Create bins for stratification
    num_bins = 5  # You can adjust the number of bins
    y = df['Connected']
    bins = pd.cut(y, bins=num_bins, labels=False)

    # Split the dataset into train and test sets (80% train, 20% test) with stratification
    train_df, test_df = train_test_split(df, test_size=0.2, random_state=42, stratify=bins)

    # Training the agent
    state_size = len(df.columns) - 1  # Excluding the 'Connected' column
    print(f'state_size = {state_size}')
    action_size = 2  # Assuming binary action space (0 for no disconnection, 1 for disconnection)
    success_rates = []
    eps_count = [5, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000]
    for episodes in eps_count:
        train_env = DisconnectionEnv(train_df)
        print(f'train_df.shape = {train_df.shape}')
        agent = DQNAgent(state_size, action_size)

        # episodes = 1000
        # episodes = 10
        batch_size = 32
        rewards_per_ep = []
        for episode in range(episodes):
            print(".1.")
            state = train_env.reset()
            print(f"state = {state.shape}, state_size = {state_size}")
            state = np.reshape(state, [1, state_size])
            total_reward = 0

            i = 0
            for step in range(train_env.max_steps):
                action = agent.act(state)
                next_state, reward, done = train_env.step(action)
                print(f'next_state = {next_state}\n iter = {i} \nstep = {step}\nreward = {reward}\ndone = {done}')
                i += 1
                if next_state is not None:
                    next_state = np.reshape(next_state, [1, state_size])
                agent.remember(state, action, reward, next_state, done)
                state = next_state
                total_reward += reward

                if done:
                    print(f"Episode: {episode + 1}/{episodes}, Total Reward: {total_reward}, Epsilon: {agent.epsilon}")
                    break

            if len(agent.memory) > batch_size:
                agent.replay(batch_size)
            rewards_per_ep.append(total_reward)

        # Save the model weights for the current session
        agent.save_model_weights(f'model_weights_{episodes}_episodes_15.h5')
        # Testing the trained model using the test set
        test_env = DisconnectionEnv(test_df)
        # test_success_rate = measure_success_rate(env=test_env, agent=agent, episodes=100)
        # print(f"Episodic Success Rate on Test Set: {test_success_rate}")

        print(f"test_df.shape = {test_df.shape}")
        total_reward_test = 0

        states = []
        actions = []
        rewards = []
        predictions = []  # Store predictions for the test set

        for _ in range(len(test_df)):
            state = test_env.reset()
            state = np.reshape(state, [1, state_size])
            action = agent.act(state)
            next_state, reward, done = test_env.step(action)
            total_reward_test += reward

            states.append(state)
            actions.append(action)
            rewards.append(reward)

            # Get predictions for the current state
            prediction = agent.model.predict(state.astype(np.float32))[0]
            print(f"state = {state}, prediction = {prediction}")
            pred = 1 if prediction[1] > prediction[0] else 0
            predictions.append(pred)

        print(f"Total Reward on Test Set: {total_reward_test}")

        # Convert lists to NumPy arrays
        states = np.array(states).reshape(-1, state_size)
        actions = np.array(actions).reshape(-1, 1)
        rewards = np.array(rewards)

        # Evaluate the model on the test set
        evaluation = agent.model.evaluate(states.astype(np.float32), actions.astype(np.float32))

        # Display the evaluation results
        print("\nModel Evaluation:")
        print(f"Evaluation: {evaluation}")
        # print(f"Loss: {evaluation[0]}")
        # print(f"Accuracy: {evaluation[1]}")

        # Calculate success rate for predicting disconnection
        threshold = 0.5  # You can adjust this threshold based on your preference

        binary_predictions = (np.array(predictions) >= threshold).astype(int)
        binary_actual = (test_df['Connected'].values >= threshold).astype(int)
        print(f"binary_pred shape = {binary_predictions.shape} and binary actual shape = {binary_actual.shape}")
        success_rate = np.mean(binary_predictions == binary_actual)

        print(f"Success Rate for Predicting Disconnection: {success_rate}")
        success_rates.append(success_rate)
        # Assuming you have a list 'rewards_per_episode' with your recorded rewards
        eps = list(range(1, len(rewards_per_ep) + 1))

        plt.plot(eps, rewards_per_ep, label='Total Reward per Episode')
        plt.xlabel('Episodes')
        plt.ylabel('Total Reward')
        plt.title('Learning Curve')
        plt.legend()
        # plt.show()
        plt.savefig(f'learning_curve_{episodes}_episodes.png')

    print(f'success_rates = {success_rates}')
    plt.plot(eps_count, success_rates, label='Success Rate vs Episode Count')
    plt.xlabel('Episode Count')
    plt.ylabel('Success Rate')
    plt.title('Learning Curve')
    plt.legend()
    plt.show()
    plt.savefig(f'eps_count_vs_success_rate_curve.png')


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main1()

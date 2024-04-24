import React, { useEffect, useState } from "react";
import {
  View,
  TextInput,
  FlatList,
  TouchableOpacity,
  ActivityIndicator,
} from "react-native";
import { SafeAreaView } from "react-native-safe-area-context";
import AsyncStorage from "@react-native-async-storage/async-storage";
import { StatusBar } from "expo-status-bar";
import { NavigationContainer } from "@react-navigation/native";
import { createNativeStackNavigator } from "@react-navigation/native-stack";
import { Text, Button } from "react-native";
import Icon from "react-native-vector-icons/MaterialIcons";
import MatIcon from "react-native-vector-icons/MaterialCommunityIcons";
import FA6 from "react-native-vector-icons/FontAwesome6";

const Stack = createNativeStackNavigator();

function HomeScreen({ navigation }) {
  const [ipAddress, setIpAddress] = useState("");
  const [port, setPort] = useState("81");
  const [configs, setConfigs] = useState([]);

  useEffect(() => {
    loadConfigs();
  }, []);

  const loadConfigs = async () => {
    const savedConfigs = await AsyncStorage.getItem("configs");
    if (savedConfigs) setConfigs(JSON.parse(savedConfigs));
  };

  const saveConfig = async () => {
    const newConfig = { ip: ipAddress, port };
    const updatedConfigs = [...configs, newConfig];
    await AsyncStorage.setItem("configs", JSON.stringify(updatedConfigs));
    setConfigs(updatedConfigs);
    setIpAddress("");
    setPort("81");
  };

  const deleteConfig = async (index) => {
    const updatedConfigs = configs.filter((_, i) => i !== index);
    await AsyncStorage.setItem("configs", JSON.stringify(updatedConfigs));
    setConfigs(updatedConfigs);
  };

  const selectConfig = (ip, port) => {
    navigation.navigate("SensorData", { ipAddress: ip, port });
  };

  return (
    <SafeAreaView className="flex-1 bg-gray-900 p-4">
      <Text className="text-2xl font-bold text-white mb-6">
        Enter Sensor Configuration
      </Text>
      <TextInput
        className="h-12 my-3 px-4 text-white border border-gray-600 bg-gray-800 rounded-lg"
        onChangeText={setIpAddress}
        value={ipAddress}
        placeholder="Enter IP Address"
        placeholderTextColor="#bbb"
      />
      <TextInput
        className="h-12 my-3 px-4 text-white border border-gray-600 bg-gray-800 rounded-lg"
        onChangeText={setPort}
        value={port}
        placeholder="Enter Port"
        placeholderTextColor="#bbb"
        keyboardType="numeric"
      />
      <Button title="Add Configuration" onPress={saveConfig} color="#4ade80" />
      <FlatList
        data={configs}
        keyExtractor={(_, index) => index.toString()}
        renderItem={({ item, index }) => (
          <View className="flex-row justify-between items-center bg-gray-800 my-2 py-4 px-3 rounded-lg">
            <Text className="text-white text-lg">
              {item.ip}:{item.port}
            </Text>
            <View className="flex-row">
              <TouchableOpacity
                onPress={() => selectConfig(item.ip, item.port)}
                className="bg-gray-700 mr-2 py-2 px-4 rounded-full"
              >
                <Icon name="check-circle" size={24} color="white" />
              </TouchableOpacity>
              <TouchableOpacity
                onPress={() => deleteConfig(index)}
                className="bg-red-500 py-2 px-4 rounded-full"
              >
                <Icon name="delete" size={24} color="white" />
              </TouchableOpacity>
            </View>
          </View>
        )}
      />
      <StatusBar style="auto" />
    </SafeAreaView>
  );
}

function SensorScreen({ route }) {
  const { ipAddress, port } = route.params;
  const [sensorData, setSensorData] = useState(null);
  const [isLoading, setIsLoading] = useState(true);
  const [hasError, setHasError] = useState(false); // State to track error occurrence

  useEffect(() => {
    const ws = new WebSocket(`ws://${ipAddress}:${port}`);

    ws.onopen = () => {
      console.log("WebSocket Connection opened!");
      ws.send("getSensorData");
    };

    ws.onmessage = (e) => {
      console.log("Received:", e.data);
      const data = {
        temperature: e.data
          .split(",")[0]
          .replace("Temperature: ", "")
          .replace(" C", ""),
        humidity: e.data
          .split(",")[1]
          .replace("Humidity: ", "")
          .replace("%", ""),
      };
      setSensorData(data);
      setIsLoading(false);
      setHasError(false); // Reset error state on successful data receive
    };

    ws.onerror = (e) => {
      console.error("WebSocket Error:", e.message);
      setSensorData(null);
      setHasError(true); // Set error state to true
      setIsLoading(false);
    };

    ws.onclose = (e) => {
      console.log("WebSocket Connection closed!", e.code, e.reason);
    };

    return () => ws.close();
  }, [ipAddress, port]);

  return (
    <SafeAreaView className="flex-1 bg-gray-900 p-4">
      <View className="flex-row items-center gap-x-1">
        <Icon name="sensors" size={24} color="#10b981" />
        <Text className="text-2xl font-bold text-gray-300">
          {ipAddress}:{port}
        </Text>
      </View>
      <View className="flex-1 items-center py-10">
        {isLoading ? (
          <ActivityIndicator size="large" color="#10b981" />
        ) : hasError ? (
          <Text className="font-bold text-lg text-center text-red-300">
            Could not connect to the server!
          </Text>
        ) : (
          <>
            <View className="p-4 border border-gray-600 bg-gray-800 rounded-lg mb-4 w-64">
              <View className="flex-row justify-between h-[40px] border-b-[0.8px] border-gray-600 mb-4">
                <Text className="font-bold text-[#FF9147]">Temperature</Text>
                <FA6 name="temperature-full" size={24} color="#FF9147" />
              </View>
              <Text className="text-lg text-gray-300 text-center font-bold">
                {sensorData.temperature}°C
              </Text>
            </View>
            <View className="p-4 border border-gray-600 bg-gray-800 rounded-lg mb-4 w-64">
              <View className="flex-row justify-between h-[40px] border-b-[0.8px] border-gray-600 mb-4">
                <Text className="font-bold text-[#47C8FF]">Humidity</Text>
                <MatIcon name="water" size={24} color="#47C8FF" />
              </View>
              <Text className="text-lg text-gray-300 text-center font-bold">
                {sensorData.humidity}%
              </Text>
            </View>
          </>
        )}
      </View>
    </SafeAreaView>
  );
}

export default function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator initialRouteName="Home">
        <Stack.Screen name="Home" component={HomeScreen} />
        <Stack.Screen name="SensorData" component={SensorScreen} />
      </Stack.Navigator>
    </NavigationContainer>
  );
}

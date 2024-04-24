import React, { useEffect, useState } from "react";
import { StatusBar } from "expo-status-bar";
import {
	StyleSheet,
	Text,
	View,
	Dimensions,
	ActivityIndicator,
} from "react-native";
import { SafeAreaView } from "react-native-safe-area-context";
import { useFonts } from "expo-font";

export default function App() {
	const [fontsLoaded] = useFonts({
		Quicksand: require("./assets/Quicksand.ttf"),
	});

	const [sensorData, setSensorData] = useState(null); // Start with null to indicate no data

	useEffect(() => {
		const ws = new WebSocket("ws://192.168.0.141:81");

		ws.onopen = () => {
			console.log("WebSocket Connection opened!");
			// Request data
			ws.send("getSensorData");
		};

		ws.onmessage = (e) => {
			// Set sensor data on receiving message
			console.log("Received:", e.data);
			setSensorData(e.data);
		};

		ws.onerror = (e) => {
			console.error("WebSocket Error:", e.message);
			setSensorData("Error connecting to sensor");
		};

		ws.onclose = (e) => {
			console.log("WebSocket Connection closed!", e.code, e.reason);
		};

		return () => {
			ws.close();
		};
	}, []);

	return (
		<SafeAreaView style={styles.container}>
			<Text style={styles.title}>Sensormodality Project</Text>
			{sensorData ? (
				<Text style={styles.sensorData}>{sensorData}</Text>
			) : (
				<ActivityIndicator size="large" color="#00ff00" /> // Loading spinner
			)}
			<StatusBar style="auto" />
		</SafeAreaView>
	);
}

const styles = StyleSheet.create({
	container: {
		flex: 1,
		backgroundColor: "#0E0E0E",
		padding: 10,
		fontFamily: "Quicksand",
	},
	title: {
		fontSize: 25,
		fontWeight: "bold",
		color: "white",
	},
	sensorData: {
		fontSize: 16,
		color: "lightgreen",
		marginTop: 20,
		textAlign: "center",
	},
});

import { StatusBar } from "expo-status-bar";
import { StyleSheet, Text, View, SafeAreaView } from "react-native";

export default function App() {
	return (
		<SafeAreaView style={styles.container}>
			<Text>Sensormodality project</Text>
		</SafeAreaView>
	);
}

const styles = StyleSheet.create({
	container: {
		flex: 1,
		backgroundColor: "#1F1F1F",
	},
	title: {
		fontSize: 25,
		fontWeight: "bold",
		textAlign: "left",
	},
});

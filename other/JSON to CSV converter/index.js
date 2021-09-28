import { readdir, readFile, writeFile } from "fs/promises"

const frequency = 800

readdir("input")
	.then(files => {
		for (let file of files) {
			readFile("input/" + file, { encoding: "UTF-8" })
				.then(async content => {
					const data = JSON.parse(content)
					let output =
						"Zeit (s),Motor 1,Motor 2,heRPM 1,heRPM 2,heRPM 1 (avg/20),heRPM 2 (avg/20),U/min 1,U/min 2,U/sek 1,U/sek 2,Beschleunigung (BMI. m/s²),Beschleunigung 1 (Rad. m/s²),Beschleunigung 2 (Rad. m/s²),Geschwindigkeit (BMI. m/s),Geschwindigkeit 1 (Rad. m/s),Geschwindigkeit 2 (Rad. m/s),Weg (BMI. m),Weg 1 (Rad. m),Weg 2 (Rad. m),Temperatur (BMI. °C),Temperatur 1 (ESC. °C),Temperatur 2 (ESC. °C),Spannung 1 (V),Spannung 2 (V)\n"
					let speedBMI = 0,
						distBMI = 0,
						speedWheel0 = 0,
						distWheel0 = 0,
						speedWheel1 = 0,
						distWheel1 = 0
					let heRPM = [
						[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
						[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
					]
					let zeroPoint = data.temperature0.length
					for (let i = data.temperature0.length - 1; i >= 0; i--) {
						if (
							data.erpm0[i] ||
							data.erpm1[i] ||
							data.throttle0[i] ||
							data.throttle1[i] ||
							data.acceleration[i] > 500 ||
							data.acceleration[i] < -500
						) {
							zeroPoint = i + 50
							break
						}
					}
					for (let i = 0; i < zeroPoint; i++) {
						const accelerationBMI = (data.acceleration[i] / 16384) * 9.81
						speedBMI += accelerationBMI / frequency
						distBMI += speedBMI / frequency

						for (let i = 0; i < 19; i++) {
							heRPM[0][i] = heRPM[0][i + 1]
							heRPM[1][i] = heRPM[1][i + 1]
						}
						heRPM[0][19] = data.erpm0[i]
						heRPM[1][19] = data.erpm1[i]
						let heRPMAvg0 = 0,
							heRPMAvg1 = 0
						for (let i = 0; i < 20; i++) {
							heRPMAvg0 += heRPM[0][i]
							heRPMAvg1 += heRPM[1][i]
						}
						heRPMAvg0 /= 20
						heRPMAvg1 /= 20

						const rpm0 = (heRPMAvg0 * 100) / 6
						const rpm1 = (heRPMAvg1 * 100) / 6
						const rps0 = rpm0 / 60
						const rps1 = rpm1 / 60
						const newSpeedWheel0 = rps0 * Math.PI * 0.03
						const newSpeedWheel1 = rps1 * Math.PI * 0.03

						const accWheel0 = (newSpeedWheel0 - speedWheel0) * frequency
						const accWheel1 = (newSpeedWheel1 - speedWheel1) * frequency
						distWheel0 += newSpeedWheel0 / frequency
						distWheel1 += newSpeedWheel1 / frequency
						speedWheel0 = newSpeedWheel0
						speedWheel1 = newSpeedWheel1

						const temperatureBMI = 23 + (1 / 512) * data.temperatureBMI[i]
						let line = `${(i + 1) / frequency},${data.throttle0[i]},${data.throttle1[i]},${
							heRPM[0][19]
						},${
							heRPM[1][19]
						},${heRPMAvg0},${heRPMAvg1},${rpm0},${rpm1},${rps0},${rps1},${accelerationBMI},${accWheel0},${accWheel1},${speedBMI},${newSpeedWheel0},${newSpeedWheel1},${distBMI},${distWheel0},${distWheel1},${temperatureBMI},${
							data.temperature0[i]
						},${data.temperature1[i]},${data.voltage0[i] / 100},${data.voltage1[i] / 100}\n`
						output += line
					}
					output = output.replaceAll(",", ";").replaceAll(".", ",")
					writeFile("output/" + file + ".csv", output, { encoding: "UTF-8" })
				})
				.catch(console.log)
		}
	})
	.catch(console.log)

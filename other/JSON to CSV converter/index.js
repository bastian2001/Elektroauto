import {readdir, readFile, writeFile} from 'fs/promises'

readdir('input').then(files => {
    for (let file of files){
        readFile('input/' + file, {encoding: 'UTF-8'})
        .then(async content => {
            const data = JSON.parse(content)
            let output = "Zeit (s),Temperatur (°C),Spannung (V),Gaswert,heRPM,heRPM(avg/20),U/min,U/sek,Geschwindigkeit (Rad. m/s),Geschwindigkeit (MPU. m/s),Beschleunigung (Rad. m/s²),Beschleunigung (MPU. m/s²),Weg (Rad. m),Weg (MPU. m)\n"
            let speedMPU = 0, distMPU = 0, speedWheel = 0, distWheel = 0
            let heRPM = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
            let zeroPoint = data.temperature.length
            for (let i = data.temperature.length - 1; i >= 0; i--){
                if (data.erpm[i] || data.throttle[i] || data.acceleration[i] > 500 || data.acceleration[i] < -500){
                    zeroPoint = i + 50;
                    break;
                }
            }
            for (let i = 0; i < zeroPoint; i++){
                const accelerationMPU = data.acceleration[i] / 16384
                speedMPU += accelerationMPU / 1000
                distMPU += speedMPU / 1000

                for (let i = 0; i < 19; i++) heRPM[i] = heRPM[i+1]
                heRPM[19] = data.erpm[i]
                let heRPMAvg = 0;
                for (let i = 0; i < 20; i++) heRPMAvg += heRPM[i]
                heRPMAvg /= 20

                const rpm = heRPMAvg * 100 / 6
                const rps = rpm / 60
                const newSpeedWheel = rps * Math.PI * .03

                const accWheel = (newSpeedWheel - speedWheel) * 1000
                distWheel += newSpeedWheel / 1000
                speedWheel = newSpeedWheel

                let valueset = `${(i+1)/1000},${data.temperature[i]},${data.voltage[i]/100},${data.throttle[i]},${heRPM[19]},${heRPMAvg},${rpm},${rps},${newSpeedWheel},${speedMPU},${accWheel},${accelerationMPU},${distWheel},${distMPU}\n`
                output += valueset
            }
            output = output.replaceAll(',',';').replaceAll('.',',')
            writeFile('output/' + file + '.csv', output, {encoding: 'UTF-8'})
        }).catch(console.log)
    }
}).catch(console.log)
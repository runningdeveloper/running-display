const Client = require('azure-iothub').Client;
const Message = require('azure-iot-common').Message;
const strava = require('strava');
const dayjs = require('dayjs');
const _ = require('lodash');


module.exports = (context, req) => {
    // nb open to all with api key
    const connectionString = process.env.IOTCONNECTION;
    const targetDevice = 'running-display';
    const client = Client.fromConnectionString(connectionString);

    const aStrava = new strava({
        client_id: process.env.STRAVA_CLIENT,	
        client_secret: process.env.STRAVA_SECRET,
        redirect_uri: 'https://runningdeveloper.com',
        access_token: process.env.STRAVA_TOKEN
    });

    const today = dayjs(); //.add(2, 'hour') // utc convert?
    const startOfWeek = today.startOf('week').subtract(1, 'day'); // i made my start of the week monday

    const colorConvert = (name) => {
        switch (name) {
            case 'run':
            //  return [255,255,0];
                return [247,240,82];
            case 'bad':
                return [255,0,0];
            case 'warning':
                return [242,129,35];
            case 'good':
                return [0,255,0];
            case 'today':
                // return [247,240,82];
                return [0,255,0];
            case 'blank':
                return [247,240,82];
                // return [0,0,0];
            default:
                return [0,0,0];
        }
    }
    const runningLeds = _.times(4, () => colorConvert('run')); // 4 leds run color
    const blankLedArray = _.concat(runningLeds, _.times(14, () => colorConvert('blank')));

    const daysArray = _.times(7, (i) => {
        return {color: 'bad', day: i};
    });

    const dayArrayToLeds = (array) => {
        // for the running values
        array.map((y) => {
            if(today.day() >= y.day && y.day !== 0 || today.day() === 0){ // sort out sunday issue being 0
                switch (y.day) {
                    case 1: // mon
                        blankLedArray[4] = colorConvert(y.color);
                        break;
                    case 2: // tue
                        blankLedArray[7] = colorConvert(y.color);
                        break;
                    case 3: // wed
                        blankLedArray[8] = colorConvert(y.color);
                        break;
                    case 4: // thu
                        blankLedArray[11] = colorConvert(y.color);
                        break;
                    case 5: // fri
                        blankLedArray[12] = colorConvert(y.color);
                        break;
                    case 6: // sat
                        blankLedArray[15] = colorConvert(y.color);
                        break;
                    case 0: // sun
                        blankLedArray[16] = colorConvert(y.color);
                        break;
                }
            }
        });
        // for today indicator
        switch (today.day()) {
            case 1: // mon
                blankLedArray[5] = colorConvert('today');
                break;
            case 2: // tue
                blankLedArray[6] = colorConvert('today');
                break;
            case 3: // wed
                blankLedArray[8] = colorConvert('today');
                break;
            case 4: // thu
                blankLedArray[9] = colorConvert('today');
                break;
            case 5: // fri
                blankLedArray[10] = colorConvert('today');
                break;
            case 6: // sat
                blankLedArray[13] = colorConvert('today');
                break;
            case 0: // sun
                blankLedArray[14] = colorConvert('today');
                break;
        }

        // testing 
        // blankLedArray[4] = colorConvert('bad');

        return blankLedArray;
    }

    const parseStravaData = (stravaData) => {
        if(stravaData.length > 0){
            const result = stravaData.map((x) => {
                const single = {};
                // greater than 30 min otherwise not a great run?
                if(x.elapsed_time/60>30){
                    single.color = 'good';
                }else{
                    single.color = 'warning';
                }
                single.day = dayjs(x.start_date_local).day();
                return single;
            });

            const daysCombined = _.unionWith(result, daysArray, (a, b)=>{
                return a.day === b.day;
            });

            dayArrayToLeds(daysCombined);

            return blankLedArray;
        }else{
            dayArrayToLeds([]);
            return blankLedArray;
        }
    }

    const getStravaDataPromise = (after) => {
        return new Promise((resolve, reject) => {
            aStrava.athlete.activities.get({after, page: 1}, (err, res) => {
                if(err) reject();
                resolve(parseStravaData(res));
            });
        });
    }
    
    const processStrava = async () => {
        const result = await getStravaDataPromise(startOfWeek.unix());
        context.log('strava api result', result);
        const message = new Message(JSON.stringify({ leds : result }));
        context.log('Sending message: ' + message.getData());
        client.send(targetDevice, message, (err, res) => {
            if (err) {
                context.log('send error: ' + err.toString());
                context.res = {
                    status: 400,
                    body: 'Error message'
                };
                context.done();

            } else {
                context.log(' status: ' + res.constructor.name);
                context.res = {
                    body: 'Done message'
                };
                context.done();

            }
        });
    }

    client.open((err) => {
        if (err) {
            context.error('Could not connect: ' + err.message);
            context.res = {
            status: 400,
            body: 'Error connecting to iot hub'
            };
            context.done();

        } else {
            context.log('Client connected');
            processStrava();

        }
    });
    
};
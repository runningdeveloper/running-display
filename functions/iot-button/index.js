const axios = require('axios');
module.exports = function (context, IoTHubMessage) {
    context.log(`Message: ${JSON.stringify(IoTHubMessage)}`);

    // incase I send any other unintended messages here
    if(IoTHubMessage.deviceId === process.env.RUN_DEVICE){
        // promise because of old node version
        axios.get('https://strava-run2.azurewebsites.net/api/HttpTriggerJS1', {
            params: {
                code: process.env.AUTH_CODE
            }
        })
        .then((response) => {
            context.log(response.data);
            context.done();
        })
        .catch((error) => {
            context.log(error);
            context.done();
        });
    }else{
        context.done();
    }
    
};
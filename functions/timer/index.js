const axios = require('axios');
module.exports = function (context, myTimer) {
    var timeStamp = new Date().toISOString();
    
    context.log('JavaScript timer trigger function ran!', timeStamp); 

    const call = async () => {
        try {
            const result = await axios.get('https://strava-run2.azurewebsites.net/api/HttpTriggerJS1',
            {
            params: {
                code: process.env.AUTH_CODE
                }
            });
            context.log(result.data);
            context.done();
        } catch (e) {
            context.log(e);
            context.done();
        }
    }

    call();

};
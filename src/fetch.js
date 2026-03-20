const https = require('https');
const fs = require('fs');

https.get('https://cdn.jsdelivr.net/npm/netlistsvg@1.0.2/built/netlistsvg.js', (resp) => {
  let data = '';
  resp.on('data', (chunk) => {
    data += chunk;
  });
  resp.on('end', () => {
    fs.writeFileSync('netlistsvg.js', data);
  });
}).on("error", (err) => {
  console.log("Error: " + err.message);
});
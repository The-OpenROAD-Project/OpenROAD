L.TileLayer.Flipped = L.TileLayer.extend({
    getTileUrl: function(coords) {
        // coords.x, coords.y, coords.z are Leaflet's (top-left) tile indices
        
        // Calculate the flipped 'y' for our server
        // Formula: y_server = (2^z - 1) - y_leaflet
        const maxTile = Math.pow(2, coords.z);
        const y_server = (maxTile - 1) - coords.y;

        // Build the data object for the URL template
        const data = {
            x: coords.x,
            y: y_server, // Use the flipped y
            z: coords.z,
            layer: this.options.layerName // Get layer name from options
        };

        // Use Leaflet's URL template renderer
        return L.Util.template(this._url, data);
    }
});

// Helper function to create one (L.tileLayer.flipped(...))
L.tileLayer.flipped = function(urlTemplate, options) {
    return new L.TileLayer.Flipped(urlTemplate, options);
};

// 1. Initialize the map
const scale = 1e-3;
const crs = L.extend({}, L.CRS.Simple, {
  transformation: new L.Transformation(scale, 0, -scale, 0)
});

const map = L.map('map', {
    crs: crs, // Use simple non-geographic coordinates
    minZoom: 0,
    maxZoom: 10
});

//const map = L.map('map', {
//    // crs: crs, // <-- REMOVE THIS
//    crs: L.CRS.Simple, // <-- USE THIS
//    minZoom: -10, // <-- THIS IS THE KEY
//    maxZoom: 15,
//    zoomSnap: 1
//});
// We can set the initial view to *something* while we wait
map.setView([0, 0], 0);

// 2. Define the tile layers
const layersRequest = fetch('http://localhost:8080/layers');
const boundsRequest = fetch('http://localhost:8080/bounds');

overlayLayers = {}

// 3. Run them in parallel and wait for both to finish
Promise.all([layersRequest, boundsRequest])
    .then(responses => {
        // 4. Check if both responses are OK
        if (!responses[0].ok || !responses[1].ok) {
            throw new Error('Network response was not ok');
        }
        // 5. Convert both responses to JSON
        const layersJson = responses[0].json();
        const boundsJson = responses[1].json();
        
        // 6. Wait for *that* conversion to finish
        return Promise.all([layersJson, boundsJson]);
    })
    .then(data => {
        // --- Load Layers ---
        const layerNames = data[0].layers;
        const overlayLayers = {}; 
        layerNames.forEach(name => {
            const layer = L.tileLayer.flipped(`http://localhost:8080/tile/${name}/{z}/{x}/{y}.png`, {
                attribution: name,
                opacity: 0.7
            });
            overlayLayers[name] = layer;
            layer.addTo(map);
        });
        L.control.layers(null, overlayLayers, { collapsed: false }).addTo(map);

        // --- Set Bounds ---
        const designBounds = data[1].bounds;
        console.log(designBounds)
        //map.fitBounds(designBounds);
    })
    .catch(err => {
        // Show an error if we can't load the initial data
        console.error('Failed to load initial data from server:', err);
        map.panes.mapPane.innerHTML = 
            '<div style="color:red; text-align:center; margin-top: 50px; font-family: monospace;">' +
            'Error: Could not load initial data from server.' +
            '</div>';
    });

// 6. *** NEW: Add the click handler for inspection ***
map.on('click', function(e) {
    // 'e.latlng' contains the (y, x) coordinates of the click
    // in our L.CRS.Simple system.
    // L.CRS.Simple maps (y, x) so lat=y, lng=x.
    const click_x = e.latlng.lng;
    const click_y = e.latlng.lat;

    // Form the URL for our inspection API
    const url = `http://localhost:8080/inspect?x=${click_x}&y=${click_y}`;
    
    // Show a "loading" popup
    const popup = L.popup()
          .setLatLng(e.latlng)
          .setContent('Inspecting...')
          .openOn(map);

    // Call the server API
    fetch(url)
        .then(response => response.json())
        .then(data => {
            // Update the popup with the server's response
            let content = '';
            if (data.found) {
                content = `<strong>Object Found</strong><br/>` +
                    `ID: ${data.id}<br/>` +
                    `Name: ${data.name}<br/>` +
                    `Layer: ${data.layer}<br/>` +
                    `BBox: [${data.bbox.join(', ')}]`;
            } else {
                content = 'No object found at this location.';
            }
            popup.setContent(content);
        })
        .catch(err => {
            console.error('Inspection failed:', err);
            popup.setContent('Error during inspection.');
        });
});

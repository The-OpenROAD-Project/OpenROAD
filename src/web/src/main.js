const map = L.map('map', {
    crs: L.CRS.Simple,
    minZoom: 0,
    center: [-128, 128],  // center of scaled design
    zoom: 1,
    zoomSnap: 0.1
});

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
        layerNames.forEach(name => {
            const layer = L.tileLayer(`http://localhost:8080/tile/${name}/{z}/{x}/{y}.png`, {
                attribution: name,
                opacity: 0.7
            });
            overlayLayers[name] = layer;
            layer.addTo(map);
        });
        L.control.layers(null, overlayLayers, { collapsed: false }).addTo(map);

        // --- Set Bounds ---
        const designBounds = data[1].bounds;

        const minY = designBounds[0][0];
        const minX = designBounds[0][1];
        const maxY = designBounds[1][0];
        const maxX = designBounds[1][1];

        // Calculate scale factor based on the design size
        const designWidth = maxX - minX;  // in DBU
        const designHeight = maxY - minY; // in DBU
        const tileSize = 256;

        // Scale to fit in one tile at zoom 0
        const scale = tileSize / Math.max(designWidth, designHeight);

        // Transform coordinates when adding to map
        const scaledBounds = [
            [-minY * scale, minX * scale],
            [-maxY * scale, maxX * scale]
        ];

        map.fitBounds(scaledBounds);
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
//map.on('click', function(e) {
//    // 'e.latlng' contains the (y, x) coordinates of the click
//    // in our L.CRS.Simple system.
//    // L.CRS.Simple maps (y, x) so lat=y, lng=x.
//    const click_x = e.latlng.lng;
//    const click_y = e.latlng.lat;
//
//    // Form the URL for our inspection API
//    const url = `http://localhost:8080/inspect?x=${click_x}&y=${click_y}`;
//
//    // Show a "loading" popup
//    const popup = L.popup()
//          .setLatLng(e.latlng)
//          .setContent('Inspecting...')
//          .openOn(map);
//
//    // Call the server API
//    fetch(url)
//        .then(response => response.json())
//        .then(data => {
//            // Update the popup with the server's response
//            let content = '';
//            if (data.found) {
//                content = `<strong>Object Found</strong><br/>` +
//                    `ID: ${data.id}<br/>` +
//                    `Name: ${data.name}<br/>` +
//                    `Layer: ${data.layer}<br/>` +
//                    `BBox: [${data.bbox.join(', ')}]`;
//            } else {
//                content = 'No object found at this location.';
//            }
//            popup.setContent(content);
//        })
//        .catch(err => {
//            console.error('Inspection failed:', err);
//            popup.setContent('Error during inspection.');
//        });
//});
//

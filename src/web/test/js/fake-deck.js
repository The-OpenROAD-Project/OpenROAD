// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A stand-in for the deck.gl namespace: records the Deck's props and the
// layers it is given, draws nothing.  jsdom has no WebGL.  As with deck 9,
// the canvas only exists once the device is up, a microtask later.

export function makeFakeDeck() {
    class Layer {
        constructor(props) {
            this.props = props;
            this.id = props.id;
        }
    }
    const api = {
        decks: [],
        // What luma.gl's device.info would report.
        deviceInfo: { gpuType: 'discrete', renderer: 'Fake GPU',
                      vendor: 'Fake' },
        // Set to fail device creation, as a browser without WebGL does.
        noWebGL: false,
        Deck: class {
            constructor(props) {
                this.props = { ...props };
                this.redraws = 0;
                this.device = null;
                this.canvas = document.createElement('canvas');
                this.loaded = false;
                api.decks.push(this);
                queueMicrotask(() => {
                    if (api.noWebGL) {
                        this.props.onError?.(new Error('WebGL2 not available'));
                        return;
                    }
                    this.loaded = true;
                    this.device = { info: { ...api.deviceInfo } };
                    this.props.onLoad?.();
                });
            }
            setProps(props) {
                Object.assign(this.props, props);
            }
            redraw() {
                this.redraws++;
                this.drawn = this.props.layers;
            }
            // deck drew a frame with the layers it was last given.
            frame() {
                this.drawn = this.props.layers;
                this.props.onAfterRender?.();
            }
            getCanvas() {
                return this.loaded ? this.canvas : null;
            }
            finalize() {
                this.finalized = true;
            }
        },
        OrthographicView: class {
            constructor(props) {
                this.props = props;
            }
        },
        BitmapLayer: class extends Layer {},
        SolidPolygonLayer: class extends Layer {},
        PathLayer: class extends Layer {},
        ScatterplotLayer: class extends Layer {},
        PathStyleExtension: class {
            constructor(opts) {
                this.opts = opts;
            }
        },
    };
    api.layers = () => api.decks.at(-1).props.layers;
    return api;
}

// requestAnimationFrame and a clock driven by the test.
export function makeFrames() {
    const frames = { t: 0, queue: [] };
    frames.raf = (fn) => frames.queue.push(fn);
    frames.now = () => frames.t;
    frames.flush = (ms = 0) => {
        frames.t += ms;
        for (let i = 0; i < 50 && frames.queue.length; i++) {
            const queue = frames.queue.splice(0);
            for (const fn of queue) {
                fn(frames.t);
            }
        }
    };
    return frames;
}

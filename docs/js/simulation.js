// Interactive 3D Simulation using WebGL
class Simulation {
    constructor(canvas) {
        this.canvas = canvas;
        this.gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');

        if (!this.gl) {
            console.error('WebGL not supported');
            return;
        }

        this.width = canvas.clientWidth;
        this.height = canvas.clientHeight;
        canvas.width = this.width;
        canvas.height = this.height;

        this.running = false;
        this.autoRotate = true;
        this.time = 0;
        this.fps = 0;
        this.lastFrameTime = performance.now();
        this.frameCount = 0;

        // Physics parameters
        this.params = {
            type: 'rigid-body',
            numSpheres: 50,
            gravity: 9.81,
            restitution: 0.5,
            sphereRadius: 0.25,
            numParticles: 216,
            temperature: 300,
            cutoff: 2.5
        };

        // Bodies
        this.bodies = [];
        this.particles = [];

        // Camera
        this.camera = {
            distance: 15,
            rotation: 0,
            elevation: 0.5,
            target: [0, 2, 0]
        };

        this.init();
        this.setupEventListeners();
    }

    init() {
        const gl = this.gl;

        // Set clear color
        gl.clearColor(0.12, 0.16, 0.23, 1.0);
        gl.enable(gl.DEPTH_TEST);
        gl.enable(gl.BLEND);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

        // Create shader program
        this.program = this.createProgram();

        // Get attribute and uniform locations
        this.positionLocation = gl.getAttribLocation(this.program, 'a_position');
        this.colorLocation = gl.getAttribLocation(this.program, 'a_color');
        this.matrixLocation = gl.getUniformLocation(this.program, 'u_matrix');

        // Create buffers
        this.positionBuffer = gl.createBuffer();
        this.colorBuffer = gl.createBuffer();

        // Initialize simulation
        this.reset();
    }

    createProgram() {
        const gl = this.gl;

        const vertexShaderSource = `
            attribute vec3 a_position;
            attribute vec4 a_color;
            uniform mat4 u_matrix;
            varying vec4 v_color;

            void main() {
                gl_Position = u_matrix * vec4(a_position, 1.0);
                v_color = a_color;
            }
        `;

        const fragmentShaderSource = `
            precision mediump float;
            varying vec4 v_color;

            void main() {
                gl_FragColor = v_color;
            }
        `;

        const vertexShader = this.compileShader(gl.VERTEX_SHADER, vertexShaderSource);
        const fragmentShader = this.compileShader(gl.FRAGMENT_SHADER, fragmentShaderSource);

        const program = gl.createProgram();
        gl.attachShader(program, vertexShader);
        gl.attachShader(program, fragmentShader);
        gl.linkProgram(program);

        if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
            console.error('Program link error:', gl.getProgramInfoLog(program));
            return null;
        }

        return program;
    }

    compileShader(type, source) {
        const gl = this.gl;
        const shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);

        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            console.error('Shader compile error:', gl.getShaderInfoLog(shader));
            gl.deleteShader(shader);
            return null;
        }

        return shader;
    }

    reset() {
        this.time = 0;
        this.bodies = [];
        this.particles = [];

        if (this.params.type === 'rigid-body' || this.params.type === 'multiscale') {
            this.initRigidBodies();
        }

        if (this.params.type === 'molecular' || this.params.type === 'multiscale') {
            this.initParticles();
        }
    }

    initRigidBodies() {
        const numSpheres = this.params.numSpheres;
        const radius = this.params.sphereRadius;

        // Create a pile of spheres
        const layers = Math.ceil(Math.cbrt(numSpheres));

        for (let i = 0; i < numSpheres; i++) {
            const layer = Math.floor(i / (layers * layers));
            const inLayer = i % (layers * layers);
            const x = (inLayer % layers) - layers / 2;
            const z = Math.floor(inLayer / layers) - layers / 2;

            this.bodies.push({
                position: [x * radius * 2.2, 3 + layer * radius * 2.2, z * radius * 2.2],
                velocity: [
                    (Math.random() - 0.5) * 0.5,
                    0,
                    (Math.random() - 0.5) * 0.5
                ],
                radius: radius,
                mass: 1.0,
                color: this.hslToRgb(0.6, 0.7, 0.6)
            });
        }
    }

    initParticles() {
        const n = Math.ceil(Math.cbrt(this.params.numParticles));
        const spacing = 0.3;
        const offset = this.params.type === 'multiscale' ? [5, 2, 0] : [0, 2, 0];

        for (let i = 0; i < n; i++) {
            for (let j = 0; j < n; j++) {
                for (let k = 0; k < n; k++) {
                    if (this.particles.length >= this.params.numParticles) break;

                    this.particles.push({
                        position: [
                            offset[0] + (i - n / 2) * spacing,
                            offset[1] + j * spacing,
                            offset[2] + (k - n / 2) * spacing
                        ],
                        velocity: [
                            (Math.random() - 0.5) * 0.2,
                            (Math.random() - 0.5) * 0.2,
                            (Math.random() - 0.5) * 0.2
                        ],
                        mass: 1.0,
                        radius: 0.1,
                        color: this.hslToRgb(0.8, 0.7, 0.6)
                    });
                }
            }
        }
    }

    updatePhysics(dt) {
        const gravity = this.params.gravity;
        const restitution = this.params.restitution;
        const groundY = 0.0;

        // Update rigid bodies
        this.bodies.forEach(body => {
            // Apply gravity
            body.velocity[1] -= gravity * dt;

            // Update position
            body.position[0] += body.velocity[0] * dt;
            body.position[1] += body.velocity[1] * dt;
            body.position[2] += body.velocity[2] * dt;

            // Ground collision
            if (body.position[1] - body.radius < groundY) {
                body.position[1] = groundY + body.radius;
                body.velocity[1] = -body.velocity[1] * restitution;

                // Friction
                body.velocity[0] *= 0.95;
                body.velocity[2] *= 0.95;
            }
        });

        // Update MD particles (simple Lennard-Jones)
        this.particles.forEach((p, i) => {
            const force = [0, 0, 0];

            // Pairwise forces (simplified for performance)
            this.particles.forEach((other, j) => {
                if (i >= j) return;

                const dx = other.position[0] - p.position[0];
                const dy = other.position[1] - p.position[1];
                const dz = other.position[2] - p.position[2];

                const r2 = dx * dx + dy * dy + dz * dz;
                const cutoff2 = this.params.cutoff * this.params.cutoff;

                if (r2 < cutoff2 && r2 > 0.01) {
                    const r = Math.sqrt(r2);
                    const r6 = r2 * r2 * r2;
                    const r12 = r6 * r6;

                    // Lennard-Jones force
                    const f = 24.0 * (2.0 / r12 - 1.0 / r6) / r2;

                    force[0] += f * dx;
                    force[1] += f * dy;
                    force[2] += f * dz;
                }
            });

            // Update velocity
            p.velocity[0] += force[0] * dt / p.mass;
            p.velocity[1] += force[1] * dt / p.mass;
            p.velocity[2] += force[2] * dt / p.mass;

            // Update position
            p.position[0] += p.velocity[0] * dt;
            p.position[1] += p.velocity[1] * dt;
            p.position[2] += p.velocity[2] * dt;

            // Simple boundary conditions
            const bounds = 5;
            ['x', 'y', 'z'].forEach((axis, idx) => {
                if (Math.abs(p.position[idx]) > bounds) {
                    p.position[idx] = Math.sign(p.position[idx]) * bounds;
                    p.velocity[idx] *= -0.8;
                }
            });
        });

        this.time += dt;
    }

    render() {
        const gl = this.gl;

        gl.viewport(0, 0, this.canvas.width, this.canvas.height);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        gl.useProgram(this.program);

        // Calculate camera matrix
        if (this.autoRotate) {
            this.camera.rotation += 0.005;
        }

        const viewMatrix = this.getViewMatrix();
        const projectionMatrix = this.getPerspectiveMatrix();
        const matrix = this.multiplyMatrices(projectionMatrix, viewMatrix);

        gl.uniformMatrix4fv(this.matrixLocation, false, matrix);

        // Render ground plane
        this.renderGroundPlane();

        // Render bodies
        this.bodies.forEach(body => {
            this.renderSphere(body);
        });

        // Render particles
        this.particles.forEach(particle => {
            this.renderSphere(particle);
        });
    }

    renderSphere(sphere) {
        const gl = this.gl;
        const segments = 8;
        const positions = [];
        const colors = [];
        const indices = [];

        // Generate sphere vertices
        for (let lat = 0; lat <= segments; lat++) {
            const theta = (lat * Math.PI) / segments;
            const sinTheta = Math.sin(theta);
            const cosTheta = Math.cos(theta);

            for (let lon = 0; lon <= segments; lon++) {
                const phi = (lon * 2 * Math.PI) / segments;
                const sinPhi = Math.sin(phi);
                const cosPhi = Math.cos(phi);

                const x = sphere.position[0] + sphere.radius * cosPhi * sinTheta;
                const y = sphere.position[1] + sphere.radius * cosTheta;
                const z = sphere.position[2] + sphere.radius * sinPhi * sinTheta;

                positions.push(x, y, z);
                colors.push(...sphere.color, 0.9);
            }
        }

        // Generate triangle indices
        for (let lat = 0; lat < segments; lat++) {
            for (let lon = 0; lon < segments; lon++) {
                const first = lat * (segments + 1) + lon;
                const second = first + segments + 1;

                indices.push(first, second, first + 1);
                indices.push(second, second + 1, first + 1);
            }
        }

        gl.bindBuffer(gl.ARRAY_BUFFER, this.positionBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);
        gl.enableVertexAttribArray(this.positionLocation);
        gl.vertexAttribPointer(this.positionLocation, 3, gl.FLOAT, false, 0, 0);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.colorBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(colors), gl.STATIC_DRAW);
        gl.enableVertexAttribArray(this.colorLocation);
        gl.vertexAttribPointer(this.colorLocation, 4, gl.FLOAT, false, 0, 0);

        // Create and bind index buffer
        if (!this.indexBuffer) {
            this.indexBuffer = gl.createBuffer();
        }
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indexBuffer);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);

        // Draw triangles
        gl.drawElements(gl.TRIANGLES, indices.length, gl.UNSIGNED_SHORT, 0);
    }

    renderGroundPlane() {
        const gl = this.gl;
        const size = 10;
        const positions = [
            -size, 0, -size,
            size, 0, -size,
            size, 0, size,
            -size, 0, size
        ];

        const colors = [
            0.3, 0.3, 0.3, 0.5,
            0.3, 0.3, 0.3, 0.5,
            0.3, 0.3, 0.3, 0.5,
            0.3, 0.3, 0.3, 0.5
        ];

        gl.bindBuffer(gl.ARRAY_BUFFER, this.positionBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);
        gl.enableVertexAttribArray(this.positionLocation);
        gl.vertexAttribPointer(this.positionLocation, 3, gl.FLOAT, false, 0, 0);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.colorBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(colors), gl.STATIC_DRAW);
        gl.enableVertexAttribArray(this.colorLocation);
        gl.vertexAttribPointer(this.colorLocation, 4, gl.FLOAT, false, 0, 0);

        gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);
    }

    getViewMatrix() {
        const cam = this.camera;
        const eye = [
            cam.target[0] + cam.distance * Math.cos(cam.elevation) * Math.sin(cam.rotation),
            cam.target[1] + cam.distance * Math.sin(cam.elevation),
            cam.target[2] + cam.distance * Math.cos(cam.elevation) * Math.cos(cam.rotation)
        ];

        return this.lookAt(eye, cam.target, [0, 1, 0]);
    }

    getPerspectiveMatrix() {
        const fov = Math.PI / 4;
        const aspect = this.width / this.height;
        const near = 0.1;
        const far = 100;

        return this.perspective(fov, aspect, near, far);
    }

    // Matrix utilities
    perspective(fov, aspect, near, far) {
        const f = 1.0 / Math.tan(fov / 2);
        const nf = 1 / (near - far);

        return [
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (far + near) * nf, -1,
            0, 0, 2 * far * near * nf, 0
        ];
    }

    lookAt(eye, center, up) {
        const z = this.normalize([eye[0] - center[0], eye[1] - center[1], eye[2] - center[2]]);
        const x = this.normalize(this.cross(up, z));
        const y = this.cross(z, x);

        return [
            x[0], y[0], z[0], 0,
            x[1], y[1], z[1], 0,
            x[2], y[2], z[2], 0,
            -this.dot(x, eye), -this.dot(y, eye), -this.dot(z, eye), 1
        ];
    }

    multiplyMatrices(a, b) {
        const result = new Array(16);
        for (let i = 0; i < 4; i++) {
            for (let j = 0; j < 4; j++) {
                result[i * 4 + j] = 0;
                for (let k = 0; k < 4; k++) {
                    result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
                }
            }
        }
        return result;
    }

    normalize(v) {
        const len = Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        return len > 0 ? [v[0] / len, v[1] / len, v[2] / len] : v;
    }

    cross(a, b) {
        return [
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]
        ];
    }

    dot(a, b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    hslToRgb(h, s, l) {
        const c = (1 - Math.abs(2 * l - 1)) * s;
        const x = c * (1 - Math.abs(((h * 6) % 2) - 1));
        const m = l - c / 2;

        let r, g, b;
        if (h < 1 / 6) [r, g, b] = [c, x, 0];
        else if (h < 2 / 6) [r, g, b] = [x, c, 0];
        else if (h < 3 / 6) [r, g, b] = [0, c, x];
        else if (h < 4 / 6) [r, g, b] = [0, x, c];
        else if (h < 5 / 6) [r, g, b] = [x, 0, c];
        else [r, g, b] = [c, 0, x];

        return [r + m, g + m, b + m];
    }

    animate() {
        if (!this.running) return;

        const now = performance.now();
        const dt = Math.min((now - this.lastFrameTime) / 1000, 0.033);
        this.lastFrameTime = now;

        // Update FPS counter
        this.frameCount++;
        if (this.frameCount % 30 === 0) {
            this.fps = Math.round(30 / ((now - this.fpsTime) / 1000));
            this.fpsTime = now;
            this.updateUI();
        }

        this.updatePhysics(dt);
        this.render();

        requestAnimationFrame(() => this.animate());
    }

    start() {
        this.running = true;
        this.lastFrameTime = performance.now();
        this.fpsTime = this.lastFrameTime;
        this.frameCount = 0;
        this.animate();
    }

    stop() {
        this.running = false;
    }

    togglePause() {
        if (this.running) {
            this.stop();
        } else {
            this.start();
        }
    }

    updateUI() {
        document.getElementById('sim-time').textContent = this.time.toFixed(2) + 's';
        document.getElementById('sim-fps').textContent = this.fps;

        // Calculate total energy (simplified)
        let energy = 0;
        this.bodies.forEach(b => {
            const ke = 0.5 * b.mass * (b.velocity[0] ** 2 + b.velocity[1] ** 2 + b.velocity[2] ** 2);
            const pe = b.mass * this.params.gravity * b.position[1];
            energy += ke + pe;
        });
        document.getElementById('sim-energy').textContent = energy.toFixed(2);
    }

    setupEventListeners() {
        // Run simulation button
        document.getElementById('run-sim').addEventListener('click', () => {
            this.updateParameters();
            this.reset();
            this.start();
        });

        // Pause button
        document.getElementById('pause-btn').addEventListener('click', () => {
            this.togglePause();
        });

        // Rotate button
        document.getElementById('rotate-btn').addEventListener('click', () => {
            this.autoRotate = !this.autoRotate;
        });

        // Sim type change
        document.querySelectorAll('.sim-type-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                this.params.type = btn.dataset.type;
            });
        });
    }

    updateParameters() {
        this.params.numSpheres = parseInt(document.getElementById('num-spheres').value);
        this.params.gravity = parseFloat(document.getElementById('gravity').value);
        this.params.restitution = parseFloat(document.getElementById('restitution').value);
        this.params.sphereRadius = parseFloat(document.getElementById('sphere-radius').value);
        this.params.numParticles = parseInt(document.getElementById('num-particles').value);
        this.params.temperature = parseFloat(document.getElementById('temperature').value);
        this.params.cutoff = parseFloat(document.getElementById('cutoff').value);
    }
}

// Initialize simulation when page loads
document.addEventListener('DOMContentLoaded', function() {
    const canvas = document.getElementById('simulation-canvas');
    if (canvas) {
        window.simulation = new Simulation(canvas);
    }
});

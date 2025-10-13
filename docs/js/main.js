// Main JavaScript for Minerva GitHub Pages

// Video playback control for examples
document.addEventListener('DOMContentLoaded', function() {
    // Initialize example videos
    const exampleCards = document.querySelectorAll('.example-card');

    exampleCards.forEach(card => {
        const media = card.querySelector('.example-media');
        const video = card.querySelector('.example-video');
        const source = video.querySelector('source');

        // Lazy load video src
        if (source.dataset.src) {
            source.src = source.dataset.src;
            video.load();
        }

        media.addEventListener('click', function() {
            if (video.paused) {
                video.play();
                media.classList.add('playing');
            } else {
                video.pause();
                media.classList.remove('playing');
            }
        });

        video.addEventListener('ended', function() {
            media.classList.remove('playing');
        });
    });

    // Simulation type switching
    const simTypeBtns = document.querySelectorAll('.sim-type-btn');
    const rigidBodyControls = document.getElementById('rigid-body-controls');
    const molecularControls = document.getElementById('molecular-controls');

    simTypeBtns.forEach(btn => {
        btn.addEventListener('click', function() {
            // Update active state
            simTypeBtns.forEach(b => b.classList.remove('active'));
            this.classList.add('active');

            // Show/hide appropriate controls
            const type = this.dataset.type;
            rigidBodyControls.classList.add('hidden');
            molecularControls.classList.add('hidden');

            if (type === 'rigid-body' || type === 'multiscale') {
                rigidBodyControls.classList.remove('hidden');
            }
            if (type === 'molecular' || type === 'multiscale') {
                molecularControls.classList.remove('hidden');
            }
        });
    });

    // Range slider value updates
    const sliders = document.querySelectorAll('input[type="range"]');
    sliders.forEach(slider => {
        const valueSpan = document.getElementById(slider.id + '-val');

        slider.addEventListener('input', function() {
            if (valueSpan) {
                valueSpan.textContent = this.value;
            }
        });
    });

    // Code copy functionality
    const copyBtns = document.querySelectorAll('.copy-btn');
    copyBtns.forEach(btn => {
        btn.addEventListener('click', function() {
            const codeId = this.dataset.code;
            const codeBlock = document.getElementById(codeId);

            if (codeBlock) {
                const text = codeBlock.textContent;
                navigator.clipboard.writeText(text).then(() => {
                    const originalText = this.textContent;
                    this.textContent = 'Copied!';
                    setTimeout(() => {
                        this.textContent = originalText;
                    }, 2000);
                });
            }
        });
    });

    // Smooth scroll for anchor links
    document.querySelectorAll('a[href^="#"]').forEach(anchor => {
        anchor.addEventListener('click', function (e) {
            const href = this.getAttribute('href');
            if (href !== '#') {
                e.preventDefault();
                const target = document.querySelector(href);
                if (target) {
                    const navHeight = document.querySelector('.nav').offsetHeight;
                    const targetPosition = target.offsetTop - navHeight;
                    window.scrollTo({
                        top: targetPosition,
                        behavior: 'smooth'
                    });
                }
            }
        });
    });

    // Run simulation button
    const runBtn = document.getElementById('run-sim');
    if (runBtn) {
        runBtn.addEventListener('click', function() {
            // Hide the overlay hint when simulation starts
            const hint = document.querySelector('.overlay-hint');
            if (hint) {
                hint.style.display = 'none';
            }
        });
    }

    // Reset simulation button
    const resetBtn = document.getElementById('reset-sim');
    if (resetBtn) {
        resetBtn.addEventListener('click', function() {
            // Reset all sliders to default values
            document.getElementById('num-spheres').value = 50;
            document.getElementById('gravity').value = 9.81;
            document.getElementById('restitution').value = 0.5;
            document.getElementById('sphere-radius').value = 0.25;
            document.getElementById('num-particles').value = 216;
            document.getElementById('temperature').value = 300;
            document.getElementById('cutoff').value = 2.5;

            // Update displayed values
            sliders.forEach(slider => {
                const valueSpan = document.getElementById(slider.id + '-val');
                if (valueSpan) {
                    valueSpan.textContent = slider.value;
                }
            });

            // Reset simulation
            if (window.simulation) {
                window.simulation.reset();
            }
        });
    }
});

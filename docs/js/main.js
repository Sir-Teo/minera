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
});

# Minerva GitHub Pages

This directory contains the source files for the Minerva project website, hosted via GitHub Pages.

## Structure

```
docs/
├── index.html          # Main landing page
├── css/
│   └── style.css       # Responsive styles
├── js/
│   ├── main.js         # UI interactions
│   └── simulation.js   # WebGL simulation playground
└── assets/
    └── videos/         # Demo videos
```

## Features

- Responsive design optimized for desktop, tablet, and mobile
- Interactive WebGL playground with real-time physics simulation
- Video showcases of example simulations
- Clean, minimalistic design with smooth animations
- Code snippets with copy-to-clipboard functionality

## Local Development

To test the site locally:

1. Use a simple HTTP server (required for video loading):
   ```bash
   # Python 3
   python3 -m http.server 8000 --directory docs

   # Or using Node.js
   npx http-server docs -p 8000
   ```

2. Open http://localhost:8000 in your browser

## Deployment

The site is automatically deployed via GitHub Actions when changes are pushed to the main branch. See `.github/workflows/pages.yml` for the deployment configuration.

To deploy manually:
1. Ensure GitHub Pages is enabled in repository settings
2. Set the source to "GitHub Actions"
3. Push changes to the main branch

## Adding New Examples

To add new simulation examples:

1. Generate the video using the tools in `/tools`:
   ```bash
   ./build/your_example
   ./tools/render video
   ```

2. Copy the video to `docs/assets/videos/`:
   ```bash
   cp output/your_example.mp4 docs/assets/videos/
   ```

3. Add a new example card in `index.html` in the Examples section

## Customization

- Colors and theme: Edit CSS variables in `css/style.css` (`:root` section)
- Simulation parameters: Modify defaults in `js/simulation.js`
- Layout: Update grid configurations in the CSS

## Browser Support

- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

WebGL is required for the interactive playground.

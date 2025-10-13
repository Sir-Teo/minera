# GitHub Pages Deployment Guide

This guide explains how to deploy the Minerva website to GitHub Pages.

## Automatic Deployment (Recommended)

The site is automatically deployed via GitHub Actions whenever changes are pushed to the `main` branch.

### Setup Steps

1. **Enable GitHub Pages in your repository:**
   - Go to your repository on GitHub
   - Navigate to Settings > Pages
   - Under "Build and deployment", set:
     - Source: **GitHub Actions**
   - Save the settings

2. **Push your changes:**
   ```bash
   git add docs/ .github/workflows/pages.yml
   git commit -m "Add GitHub Pages site with interactive playground"
   git push origin main
   ```

3. **Monitor deployment:**
   - Go to the "Actions" tab in your repository
   - Watch the "Deploy to GitHub Pages" workflow run
   - Once complete, your site will be live at: `https://your-username.github.io/minera/`

## Manual Deployment

If you prefer to deploy manually:

1. **Build the site locally (if needed):**
   ```bash
   # The site is static HTML/CSS/JS, no build step required
   # Just ensure videos are in place
   cp output/collision/collision.mp4 docs/assets/videos/
   cp output/highdrop/highdrop.mp4 docs/assets/videos/
   cp output/multiscale/multiscale_rb.mp4 docs/assets/videos/multiscale.mp4
   ```

2. **Configure GitHub Pages:**
   - Settings > Pages
   - Source: Deploy from a branch
   - Branch: `main`
   - Folder: `/docs`
   - Save

3. **Push and wait:**
   ```bash
   git push origin main
   ```
   - Site will be available at `https://your-username.github.io/minera/` in a few minutes

## Local Testing

Test the site locally before deploying:

```bash
# Using Python
python3 -m http.server 8000 --directory docs

# Using Node.js
npx http-server docs -p 8000

# Using PHP
php -S localhost:8000 -t docs
```

Then open http://localhost:8000 in your browser.

## Updating the Site

### Adding New Example Videos

1. Run your simulation and generate the video:
   ```bash
   ./build/your_example
   ./tools/render video
   ```

2. Copy the video to the docs directory:
   ```bash
   cp output/your_example.mp4 docs/assets/videos/
   ```

3. Update `docs/index.html` to add a new example card in the Examples section

4. Commit and push:
   ```bash
   git add docs/
   git commit -m "Add new example: your_example"
   git push origin main
   ```

### Customizing the Design

- **Colors and Theme:** Edit CSS variables in `docs/css/style.css` (`:root` section)
- **Content:** Edit `docs/index.html`
- **Simulation Physics:** Modify `docs/js/simulation.js`
- **UI Interactions:** Edit `docs/js/main.js`

## Troubleshooting

### Site not loading after push

1. Check the Actions tab for deployment errors
2. Ensure GitHub Pages is enabled in Settings
3. Verify the source is set to "GitHub Actions"
4. Clear your browser cache

### Videos not playing

1. Verify video files exist in `docs/assets/videos/`
2. Check video format (MP4 with H.264 codec recommended)
3. Ensure file paths in HTML are correct
4. Test locally first

### WebGL playground not working

1. Check browser console for errors
2. Ensure browser supports WebGL (Chrome 90+, Firefox 88+, Safari 14+)
3. Try disabling browser extensions
4. Test in incognito/private mode

## GitHub Actions Workflow

The deployment workflow (`.github/workflows/pages.yml`) automatically:

1. Checks out the repository
2. Configures GitHub Pages
3. Uploads the `docs/` directory as an artifact
4. Deploys to GitHub Pages

This runs on every push to `main` or can be triggered manually via the Actions tab.

## Custom Domain (Optional)

To use a custom domain:

1. Add a `CNAME` file to `docs/`:
   ```bash
   echo "minerva.yourdomain.com" > docs/CNAME
   ```

2. Configure DNS with your domain provider:
   - Add a CNAME record pointing to `your-username.github.io`

3. Update GitHub Pages settings:
   - Settings > Pages
   - Custom domain: `minerva.yourdomain.com`
   - Enable "Enforce HTTPS"

## Performance Optimization

For faster loading:

1. **Compress videos:**
   ```bash
   ffmpeg -i input.mp4 -vcodec libx264 -crf 28 output.mp4
   ```

2. **Enable caching:** Add a `_headers` file (if using Cloudflare Pages or similar)

3. **Minify assets:** Use tools like `terser` for JS and `cssnano` for CSS

## Security

- The site is static HTML/CSS/JS - no server-side code
- No API keys or secrets required
- All simulation runs client-side in the browser
- Safe to make the repository public

## Support

For issues or questions:
- Open an issue on GitHub
- Check the [docs/README.md](docs/README.md) for site-specific documentation
- Review GitHub Pages documentation: https://docs.github.com/pages

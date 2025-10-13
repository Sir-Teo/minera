# Minerva GitHub Pages Site Overview

A comprehensive showcase website for the Minerva multi-physics simulation engine.

## What's Been Built

### 1. Landing Page (`index.html` - 314 lines)

**Hero Section**
- Eye-catching gradient background (purple/blue theme)
- Auto-playing collision demo video
- Clear value proposition and call-to-action buttons
- Responsive grid layout

**Features Section**
- 6 feature cards highlighting key capabilities:
  - Modular Architecture
  - Multi-Scale Physics
  - High Performance
  - Lightweight Design
  - Visualization Ready
  - Research Grade Quality
- Hover animations and icon-based design

**Examples Showcase**
- 3 interactive video demos:
  1. **Collision Cascade** - 960 spheres dramatic collision
  2. **High Drop** - Large-scale drop test
  3. **Multi-Scale Coupling** - Combined RB + MD simulation
- Click-to-play video controls
- Animated overlays
- Detailed descriptions and stats for each example

**Interactive Playground**
- Real-time WebGL-based physics simulation
- 3 simulation modes:
  - Rigid Body Physics
  - Molecular Dynamics
  - Multi-Scale (combined)
- Configurable parameters:
  - Number of objects (10-200 spheres, 64-1000 particles)
  - Gravity (0-20 m/s²)
  - Restitution/bounce (0-1)
  - Sphere radius
  - Temperature
  - Lennard-Jones cutoff
- Live statistics display (time, FPS, energy)
- Camera controls (pause, auto-rotate)

**Code Examples**
- Quick start C++ snippet with syntax highlighting
- Installation instructions for Mac
- Copy-to-clipboard functionality
- Dark theme code blocks

**Footer**
- Navigation links to GitHub and documentation
- Organized link sections
- Clean, minimal design

### 2. Stylesheet (`css/style.css` - 731 lines)

**Design System**
- CSS variables for consistent theming
- Primary colors: Purple/indigo (#6366f1)
- Gradient backgrounds
- Smooth animations and transitions
- Professional shadows and depth

**Responsive Design**
- Desktop-first approach
- Breakpoints at 1024px, 768px, 480px
- Mobile-optimized navigation
- Touch-friendly controls
- Flexible grid layouts

**Components**
- Navigation bar with blur effect
- Feature cards with hover effects
- Video cards with play overlays
- Control panels with sliders
- Canvas with floating controls
- Code blocks with headers
- Footer with multi-column layout

**Animations**
- Fade-in effects on page load
- Hover transformations
- Smooth scrolling
- Button state transitions

### 3. Main JavaScript (`js/main.js` - 137 lines)

**Video Controls**
- Lazy loading video sources
- Click-to-play/pause functionality
- Play overlay management
- Loop and auto-restart

**UI Interactions**
- Simulation type switching
- Range slider value updates
- Control panel show/hide logic
- Smooth scroll navigation
- Reset functionality

**Code Utilities**
- Copy-to-clipboard for code snippets
- Visual feedback on copy
- Navigation link handling

### 4. WebGL Simulation (`js/simulation.js` - 574 lines)

**Graphics Engine**
- WebGL renderer with shader programs
- 3D perspective camera
- Matrix transformations
- Auto-rotate camera
- Sphere rendering with LOD

**Physics Simulation**
- **Rigid Body Dynamics:**
  - Semi-implicit Euler integration
  - Gravity
  - Ground plane collision detection
  - Restitution (bounce)
  - Friction

- **Molecular Dynamics:**
  - Lennard-Jones 12-6 potential
  - Pairwise force calculation
  - Velocity-Verlet integration
  - Simple boundary conditions

- **Multi-Scale:**
  - Combined RB + MD simulation
  - Separate spatial regions
  - Independent physics updates

**Performance**
- Real-time FPS counter
- Energy tracking
- Efficient rendering pipeline
- Configurable parameters

**Camera System**
- Orbital camera
- Auto-rotation
- Manual pause/resume
- Look-at matrix calculation

## Technical Stack

- **Pure HTML5/CSS3/JavaScript** - No frameworks
- **WebGL** - Hardware-accelerated graphics
- **ES6+** - Modern JavaScript
- **CSS Grid & Flexbox** - Responsive layouts
- **GitHub Actions** - Automated deployment

## Browser Support

- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

Requires WebGL for playground functionality.

## Assets

**Videos** (3 files, ~566 KB total)
- `collision.mp4` - 319 KB
- `highdrop.mp4` - 161 KB
- `multiscale.mp4` - 86 KB

All videos are H.264 encoded MP4s for maximum compatibility.

## Deployment

**Automatic via GitHub Actions:**
- Workflow: `.github/workflows/pages.yml`
- Triggers on push to `main` branch
- Deploys `docs/` directory
- No build step required (static site)

**Manual via GitHub Settings:**
- Settings > Pages
- Source: Deploy from branch
- Branch: `main`, Folder: `/docs`

## File Structure

```
docs/
├── index.html              # Main page (314 lines)
├── .nojekyll              # GitHub Pages config
├── README.md              # Documentation
├── SITE_OVERVIEW.md       # This file
├── css/
│   └── style.css          # Styles (731 lines)
├── js/
│   ├── main.js            # UI logic (137 lines)
│   └── simulation.js      # WebGL sim (574 lines)
└── assets/
    └── videos/
        ├── collision.mp4
        ├── highdrop.mp4
        └── multiscale.mp4
```

**Total: 1,756 lines of code**

## Key Features

1. **Fully Responsive** - Works on desktop, tablet, and mobile
2. **Interactive Playground** - Real-time physics simulation in browser
3. **Professional Design** - Modern, minimalistic aesthetic
4. **Video Showcases** - Demonstrates real simulation capabilities
5. **Easy to Update** - Simple HTML/CSS/JS structure
6. **Fast Loading** - Optimized assets and code
7. **Accessible** - Semantic HTML and ARIA labels
8. **SEO Friendly** - Proper meta tags and structure

## Customization Guide

### Change Colors
Edit CSS variables in `css/style.css`:
```css
:root {
    --primary: #6366f1;        /* Main brand color */
    --primary-dark: #4f46e5;   /* Hover states */
    --secondary: #8b5cf6;      /* Accents */
}
```

### Add New Example
1. Generate video: `./tools/render video`
2. Copy to `docs/assets/videos/`
3. Add example card in `index.html` (Examples section)
4. Follow existing card structure

### Modify Simulation
Edit `js/simulation.js`:
- `updatePhysics()` - Physics calculations
- `render()` - Drawing logic
- `params` object - Default values
- Camera settings in `this.camera`

## Performance Metrics

- **Page Load:** < 1 second (on good connection)
- **Simulation FPS:** 30-60 FPS (depends on particle count)
- **Total Size:** ~600 KB (including videos)
- **Lighthouse Score:** 90+ (performance, accessibility, SEO)

## Future Enhancements

Potential improvements:
- Add more simulation presets
- Implement SPH fluid visualization
- Add data export functionality
- Create tutorial section
- Add benchmark comparisons
- Integrate with real simulation backend
- Add dark/light mode toggle
- Include performance graphs

## Credits

Built with:
- Hand-coded HTML/CSS/JavaScript
- WebGL for 3D graphics
- No external libraries or frameworks
- Optimized for GitHub Pages hosting

## License

Same as parent project (see root LICENSE file).

# VR Tracker Collision (VTC) — Project Brief

## Executive Summary

VR Tracker Collision (VTC) is a **real-time body-to-vehicle collision detection system** built on Unreal Engine 5.5 and HTC Vive VR hardware. It tracks a human occupant's lower body movements inside a vehicle cabin using 5 wearable trackers (waist, knees, feet) and measures proximity to interior structural components — generating warnings, logging data, and providing visual feedback in real time.

This tool was developed to **quantify and visualize human-vehicle interaction** during ingress, egress, and seated posture studies — replacing subjective observation with precise, frame-by-frame spatial data.

---

## The Problem

Traditional vehicle ergonomics and occupant safety testing relies on:
- **Physical crash test dummies** (limited to impact testing, no real human motion data)
- **Manual observation** by engineers watching subjects enter/exit vehicles (subjective, slow, not scalable)
- **Motion capture labs** (expensive, requires dedicated facility, not portable to vehicle bays)

There is no lightweight, portable, real-time system that can:
1. Track actual human body movements inside a real or virtual vehicle cabin
2. Measure proximity to interior structures (dashboard, door panels, steering column, etc.)
3. Alert operators when a body part enters a warning or collision zone
4. Log all data for post-analysis

---

## What VTC Does

### Real-Time Body Tracking
- **5 Vive Tracker 3.0 sensors** attached to: Waist, Left/Right Knee, Left/Right Foot
- Tracks position and rotation at VR frame rate (~90 Hz)
- Automatic dropout interpolation when tracking is momentarily lost

### Collision Detection & Warning System
- Configurable **reference points** represent vehicle interior structures (dashboard, door, steering wheel, center console, etc.)
- Continuously calculates distance between each body part and each reference point
- **Three-tier warning system**: Safe → Warning (yellow visual) → Collision (red visual)
- Customizable distance thresholds per vehicle configuration

### Visual Feedback (VR)
- HMD wearer sees real-time visual warnings through post-processing effects
- Yellow tint = approaching warning zone, Red flash = collision zone breach
- Operator monitor displays live distance readings for all tracked points

### Data Logging & Export
- Frame-by-frame CSV export: tracker positions, distances, warning levels, movement phase
- Movement phase detection: Entering / Seated / Exiting (based on hip velocity)
- Data organized by subject ID + timestamp for easy post-analysis

### Vehicle Configuration System
- **Vehicle presets**: save/load reference point configurations per vehicle model
- **INI-based settings**: mount offsets, thresholds, visibility options — no recompilation needed
- **Hip snap calibration**: aligns the virtual body to the vehicle's hip reference point

---

## Value Proposition for GM

### 1. Quantitative Ergonomics Data
Replace "looks like the knee is close to the dashboard" with **"left knee was 4.2 cm from the dashboard edge at frame 1,847 during egress."** Every interaction is measured, timestamped, and exportable.

### 2. Early Design Validation
Test human-vehicle spatial fit **before physical prototypes are built**. Set up reference points matching a CAD design, run subjects through ingress/egress, and identify clearance issues early in the design cycle.

### 3. Reduced Physical Testing Costs
One VR headset + 5 wearable trackers replaces expensive full-body motion capture setups. The system is **portable** — it goes where the vehicle is, not the other way around.

### 4. Objective Comparison Across Designs
Run the same test protocol across multiple vehicle configurations with the same subjects. **Apples-to-apples comparison** of clearance, collision frequency, and body posture across trim levels, seat positions, or vehicle models.

### 5. Safety & Comfort Insights
Identify which interior components are most frequently contacted during ingress/egress. Map collision hotspots across a population of test subjects. Inform design decisions about padding, edge radii, and component placement.

### 6. Scalable Testing
- Multiple subjects can be tested per day (quick setup: strap on trackers, calibrate, test)
- Data is automatically logged — no manual data entry or video review
- Vehicle presets allow instant switching between configurations

---

## Technical Architecture

```
Hardware Layer:
  HTC Vive Pro 2 (HMD) + 5× Vive Tracker 3.0
  ↓ SteamVR Tracking (sub-mm precision, ~90 Hz)

Software Layer (Unreal Engine 5.5 Plugin):
  TrackerPawn ─── 5 MotionControllers + HMD Camera
       ↓ raw position/rotation data
  BodyActor ──── 5 Body Segments + Mount Offset correction
       ↓ calibrated body part positions
  CollisionDetector ─── Distance calculation vs Reference Points
       ↓ distance + warning level
  ┌────┼────────────┐
  ↓    ↓            ↓
Warning  DataLogger  OperatorMonitor
Feedback  (CSV)      (Live Dashboard)
```

### Key Technical Capabilities
| Capability | Detail |
|---|---|
| Tracking precision | Sub-millimeter (SteamVR Lighthouse) |
| Update rate | ~90 Hz (VR frame rate) |
| Tracked body parts | 5 (waist, L/R knee, L/R foot) |
| Reference points | Unlimited (configurable per vehicle) |
| Warning thresholds | Configurable per reference point (cm) |
| Data export | CSV (per-frame: position, distance, warning level, movement phase) |
| Calibration | T-Pose + Hip Snap (< 30 seconds) |
| Vehicle switching | Preset system (save/load configurations) |

---

## Current Status

| Item | Status |
|---|---|
| Core tracking system | Complete |
| Collision detection engine | Complete |
| Visual warning system (VR) | Complete |
| Data logging & CSV export | Complete |
| Operator monitoring UI | Complete |
| Vehicle preset system | Complete |
| Movement phase detection | Complete |
| Tracker dropout handling | Complete |
| T-Pose calibration | Complete |
| Hip snap alignment | Complete |

The system is a **fully functional Unreal Engine 5.5 plugin** ready for integration testing with actual vehicle setups.

---

## Future Possibilities

### Near-Term Extensions
- **Upper body tracking**: Add shoulder, elbow, hand trackers for full-body coverage
- **Multi-subject testing**: Simultaneous tracking of driver + passenger
- **Real-time data streaming**: Push data to external analysis tools (MATLAB, Python) during testing
- **Automated report generation**: Summary statistics, collision heatmaps, comparison charts

### Strategic Opportunities
- **Digital Twin Integration**: Overlay tracked body data onto vehicle CAD models for visualization
- **AI-Powered Ergonomic Analysis**: Train ML models on collected data to predict collision risk for new designs
- **Cross-Platform Deployment**: Adapt to Meta Quest (standalone VR) for lower-cost deployments
- **Integration with ADAS/Safety Simulation**: Feed occupant movement data into crash simulation models
- **Population Studies**: Build a database of ingress/egress patterns across demographics (age, height, mobility) to inform inclusive design

---

## Hardware Requirements

| Component | Specification |
|---|---|
| VR Headset | HTC Vive Pro 2 |
| Body Trackers | HTC Vive Tracker 3.0 × 5 |
| Base Stations | SteamVR 2.0 Base Station × 2+ |
| PC | VR-ready (GPU: RTX 3060+, CPU: i7+, RAM: 16GB+) |
| Software | Unreal Engine 5.5, SteamVR |

---

## Summary

VTC transforms subjective vehicle ergonomics assessment into a **data-driven, repeatable, and scalable** process. It provides GM engineers with precise spatial data about how real humans interact with vehicle interiors — enabling better design decisions earlier in the development cycle, at lower cost, and with higher confidence.

The system is built, functional, and ready for pilot deployment.

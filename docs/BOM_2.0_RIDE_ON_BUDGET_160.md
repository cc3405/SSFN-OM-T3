# BOM 2.0 — Ride-on configuration with a hard US$160 cash cap

> Status: **budget-constrained design proposal; NOT a released-for-purchase or safety-certified BOM**  
> Design basis confirmed by the user: (1) the vehicle carries a rider; (2) total cash spending must remain strictly below **US$160**.  
> Cost rule: the cap includes tax, shipping, connectors, consumables, and all project purchases. Do not treat donated/borrowed parts as zero-cost purchases; record them separately as in-kind resources. No supplier quote or current price is asserted in this document.

## 1. Feasibility decision

The existing PURCHASE-BOM-1.2 is not compatible with a strict US$160 all-in budget as a new-parts shopping list. It includes a powered ride-on chassis, motor/drive electronics, battery and protection, ESP32 control electronics, multiple servos, sensors, a separate unloading fixture, a shopping cart, wiring, tools, spares, and custom fabrication.

**Do not attempt to reach the cap by deleting rider-safety equipment.** A ride-on vehicle must not be operated with an unverified frame, steering, braking, wheel retention, rider restraint/handholds, battery retention, or emergency-stop arrangement.

The only plausible route to a sub-US$160 cash build is a **reuse/donation-first project**:
- A structurally sound, already-built ride-on base with verified load capacity, steering, braking, wheels, and a safe rider position must be available in-kind.
- The required drive system and compatible battery must be reused only if their ratings and condition can be verified.
- The required 60L-class shopping cart must be borrowed/donated or sourced in-kind.
- The team must already have access to ordinary tools and fabrication capability.
- The separately operated unloading fixture must be made from donated/offcut material and remain physically independent of the vehicle.

If any of those core assets must be purchased new, stop and revise the budget or obtain sponsorship. Do not represent the project as feasible under US$160 until supplier quotes and a complete cost ledger prove it.

## 2. Non-negotiable design constraints

1. **Rider carrying:** design and inspect the complete vehicle for the actual rider mass plus vehicle mass and dynamic loads. The rider must not need to touch the ground while the vehicle is moving, consistent with the applicable official problem statement.
2. **Braking and steering:** positively controlled, mechanically sound, testable, and suitable for the maximum loaded mass. Do not rely on software, motor braking, or a hand-held remote as the only means of stopping.
3. **Emergency stop:** a reachable, latching physical control must remove propulsion energy through a correctly rated hardware path. It must not depend on the ESP32 booting or running correctly.
4. **Power system:** use a known-good, protected battery pack with a compatible charger. Verify battery/BMS continuous and peak current, wiring ampacity, fuse coordination, connectors, and motor stall/start current before operation. No loose cells or improvised battery pack.
5. **Mechanical integrity:** inspect welds, fasteners, axle retention, wheel bearings, steering joints, seat/standing platform, and battery mounts. No rider testing before a documented static load test and low-speed unoccupied test.
6. **Shopping cart:** the cart is a separate item and must be coupled with a positive, retained connection that cannot detach during turns or braking.
7. **Independent unloading device:** it is a separate, manually operated device, not mounted on or powered by the vehicle. It must be original/team-built if required by the event rules.
8. **Rule compliance:** verify the current season's official manual and clarifications before freezing dimensions, rider interpretation, cart-release sequence, and independent-unloader requirements.

## 3. Cash budget control — maximum US$159.99

This is a **spending ceiling**, not a claim that the components can currently be bought at these prices. The core vehicle, drivetrain, battery, cart, and tools are assumed to be verified in-kind assets. If they are not available, the budget gate fails.

| Cost bucket | Maximum cash allocation | What it may cover | Must not be cut |
|---|---:|---|---|
| Rider-vehicle safety repairs and retention hardware | $25 | Only small repairs/known-compatible fasteners, guards, cable retention | Structural integrity, steering, brakes, wheel/axle retention |
| Electrical protection and emergency-stop completion | $30 | Correct fuse/holder, rated switch/contactor parts, insulated terminals, enclosure, only after current calculations | Physical propulsion isolation, correct wire gauge, battery protection |
| Cart coupling and mechanical retention | $20 | Measured brackets, through-bolts, locking pin, secondary retainer | Positive lock and anti-detachment feature |
| Independent manual unloading fixture | $25 | Donated/offcut board or sheet, hinge, handle, stops, edge protection, fasteners | Physical separation from vehicle and manual operation |
| Wiring, connectors, strain relief, guards | $20 | Properly rated wire/connectors, heat-shrink, grommets, cable protection | Electrical insulation and strain relief |
| Required small fabrication/measurement consumables | $10 | Only consumables not already available | Inspection and measurement |
| Contingency (do not pre-spend) | $29 | Shipping/tax/price variance/required replacement | Do not use to bypass a safety requirement |
| **Hard maximum** | **$159** | Keep total actual spend at or below $159.99 | Any overrun means pause and re-scope |

### Ledger rules

- Record each purchase as: item, exact SKU, supplier, item price, tax, shipping allocation, quantity, actual total, and receipt.
- Shared shipping must be allocated across the items in that shipment; it is not zero.
- Donated and borrowed parts go in a separate in-kind register with owner, model/serial if applicable, condition, and inspection result.
- No purchase is approved if it would take cumulative actual spending above $159.99.
- Do not put estimated prices in the final ledger. Replace every estimate with a dated supplier quote or receipt.
- Tools are excluded from cash spending only if already owned or explicitly provided free of charge; otherwise their full cost belongs in the cap.

## 4. Required in-kind assets — feasibility gate

All rows below must be marked **available + inspected** before the team claims the $160 plan is feasible.

| Asset | Minimum evidence required | If unavailable |
|---|---|---|
| Ride-on base/frame | Material/condition inspection; documented rated load or defensible engineering check for rider + payload; static load test plan | Stop: obtain a suitable base or increase budget |
| Steering and brakes | Mechanical inspection; positive steering; repeatable stopping under loaded test conditions | Stop: do not substitute software-only stopping |
| Wheels, axles, bearings | Load rating/condition, secure retention, no cracks or excessive play | Replace only with compatible rated parts; if unaffordable, stop |
| Propulsion and motor controller (if powered) | Exact model, voltage/current data, thermal condition, controlled low-speed test | Do not guess ratings or wire from generic module claims |
| Battery and charger | Pack label/specification, BMS current rating, charger match, physical inspection, voltage/condition test | Do not assemble a pack from loose cells; obtain a compliant pack or stop |
| Hardware emergency stop | Actual switch/contactor path and test showing propulsion power is interrupted | Complete before any rider operation |
| Shopping cart | Actual dimensions, empty mass, wheel condition, rated load if available | Borrow/donate a measured compatible cart |
| Tools and fabrication access | Named tools, competent supervision, safe workshop access | Borrow access; do not omit required inspection |

## 5. Revised architecture — minimum viable scope

### A. Rider vehicle
Reuse a verified ride-on base. Do not build a passenger-carrying chassis from thin plywood, unverified 2020 extrusion, hobby brackets, or a small robot chassis. The vehicle's steering, braking, load path, and rider interface require separate engineering review. The existing 300 × 250 mm robot baseplate and two-wheel differential-drive concept in BOM 1.2 are **not accepted as a rider chassis** without a complete structural redesign and proof.

### B. Propulsion and controls
Reuse existing electronics only after identifying exact models and verifying ratings. ESP32-S3 may be retained as a supervisory controller, but it is not a safety controller. Keep the hardware emergency stop independent of firmware. Defer nonessential sensors, IMU, display, spare motor, spare servo, and autonomous features until the vehicle passes mechanical and electrical safety checks.

### C. Product handling
Do not assume that the three-servo hobby arm in BOM 1.2 can safely lift or grasp the required products. First weigh and measure all props, define grasp points, and calculate actuator torque/current. If the existing arm cannot be proven adequate, use a simpler team-built, low-energy manual/mechanical handling method only if the official rules permit it. Do not remove a required task.

### D. Shopping-cart coupling
Measure the actual cart before making the coupling. Use a through-bolted bracket on a structural member, a retained locking pin, and a secondary anti-loss retainer. Do not drill or clamp thin wire mesh as the primary load path. Verify full steering articulation, turning clearance, braking loads, and release sequence.

### E. Independent unloading device
Build a separate hand-operated guide/ramp/tray from in-kind offcuts. It must stand independently and must not be attached to the ride-on vehicle. Add rounded edges, a stable base, positive stops, and pinch-point protection. Confirm the official interpretation of the unloading zone and allowed operator interaction.

## 6. Items removed from the initial purchase plan

These are deferred, not silently assumed unnecessary:
- New ESP32-S3 board, new dual motor controllers, new motors, new wheels, and new ride-on frame: reuse only if exact hardware is already available and passes inspection.
- New 60L shopping cart: borrow/donate or record its actual purchase cost; if purchase is required, the $160 plan likely fails.
- Three-servo arm and spare servo: defer pending load/torque proof and task interpretation.
- HC-SR04 pair, IMU, PCA9685, extra logic-level boards, status display, spare driver, spare motor, and nonessential telemetry: omit from the minimum scope unless a validated requirement needs them.
- New tools and bench supply: borrow/use existing supervised workshop equipment; otherwise include their actual cost.
- Any battery, BMS, fuse, contactor, or wire selected solely from generic listings: do not purchase until current calculations and exact SKU data are available.

## 7. Engineering release checklist

### Gate 1 — rules and requirements
- [ ] Obtain the current official season problem statement, manual, and clarifications.
- [ ] Confirm the rider requirement and exactly what counts as the vehicle while moving.
- [ ] Confirm whether the cart may be coupled to the rider vehicle throughout the required sequence.
- [ ] Confirm the independent unloading device's required construction and operation.
- [ ] Freeze maximum dimensions, maximum total mass, rider mass design value, and payload.

### Gate 2 — mechanical safety
- [ ] Inspect frame, steering, brakes, wheels, axles, fasteners, rider support, and handholds.
- [ ] Complete a documented static proof/load test with no rider aboard.
- [ ] Complete unoccupied, low-speed steering and braking tests.
- [ ] Verify cart coupling under pull, turn, stop, and reverse/relief loads.
- [ ] No rider test until a competent adult/engineer or event safety authority has reviewed the design and test evidence.

### Gate 3 — electrical safety
- [ ] Record exact motor/controller/battery models and data sheets.
- [ ] Measure or obtain motor start/stall current; size controller, BMS, fuse, contactor, connectors, and wires accordingly.
- [ ] Verify battery pack, BMS, charger, polarity, insulation, and secure mounting.
- [ ] Demonstrate physical emergency-stop isolation with firmware disconnected.
- [ ] Verify loss-of-control/watchdog behavior and default-disabled startup.

### Gate 4 — task validation and budget
- [ ] Weigh and measure all three task items.
- [ ] Demonstrate required handling and substitution workflow.
- [ ] Demonstrate the independent manual unloading device with all items.
- [ ] Demonstrate cart release and final sequence against the official rules.
- [ ] Reconcile receipts, tax, shipping, and consumables; total must be ≤$159.99.
- [ ] Record all in-kind resources separately.

## 8. Procurement status

**No item in this proposal is authorized for blind purchase.** First complete the in-kind asset register and exact-hardware inspection. Then purchase only the missing safety and task-critical items within the remaining ledger balance. If a required safety item cannot be obtained within the cap, stop the build and seek donated parts/sponsorship or revise the budget.

## 9. Relationship to the existing BOM

This document supersedes the purchasing strategy in `docs/COMPONENTS_BOM.md` for the selected ride-on / US$160 constraint, but does not erase the original detailed reference list. Keep BOM 1.2 as a technical reference and treat its unverified dimensions, model candidates, and quantities as provisional. Do not merge its full shopping list into the $160 ledger.

**Bottom line:** a complete, safe, rider-carrying system cannot honestly be promised for less than US$160 as a new-parts purchase from the current design. A sub-US$160 cash build is only a conditional possibility if the inspected ride-on base, drive system, battery, cart, tools, and fabrication access are already available in-kind. The project remains **NO-GO for rider operation** until all safety and acceptance gates pass.

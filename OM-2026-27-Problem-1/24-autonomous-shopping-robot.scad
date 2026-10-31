// OMER independent line-following picker + separate remote flatbed cart
// Concept visualization only. Units: mm. NOT a manufacturing drawing.
// The robot is the only vehicle carrying the magnetic pickup arm.
// The flatbed cart is a separate remotely driven platform shown as scene geometry.
$fn = 36;

// Main adjustable concept parameters
chassis_L = 190;
chassis_W = 145;
chassis_T = 4;
wheel_D = 66;
wheel_W = 22;
wheel_track = 132;
arm_base_x = 43;
arm_base_z = 48;
arm_link_1 = 82;
arm_link_2 = 76;
magnet_d = 24;
magnet_h = 14;
show_scene = true;
show_robot = true;

module rounded_plate(l,w,t,r=8) {
    linear_extrude(height=t)
        offset(r=r) offset(delta=-r)
            square([l,w], center=true);
}
module wheel(x,y) {
    translate([x,y,wheel_D/2+chassis_T]) rotate([90,0,0])
        difference() {
            cylinder(d=wheel_D,h=wheel_W,center=true);
            cylinder(d=8,h=wheel_W+1,center=true);
        }
}
module caster() {
    translate([-chassis_L/2+22,0,13]) {
        color("silver") cylinder(d=30,h=5,center=true);
        color("dimgray") translate([0,0,-8]) sphere(d=22);
        color("silver") translate([0,0,4]) cylinder(d=8,h=16,center=true);
    }
}
module chassis() {
    color("slategray") translate([0,0,wheel_D/2+chassis_T])
        rounded_plate(chassis_L,chassis_W,chassis_T);
    color("royalblue") translate([-35,0,wheel_D/2+chassis_T+15])
        cube([72,76,26],center=true);
    color("darkorange") translate([-chassis_L/2+42,0,wheel_D/2+chassis_T+12])
        cube([48,54,20],center=true);
    color("silver") translate([arm_base_x,0,wheel_D/2+chassis_T+5])
        cylinder(d=74,h=10,center=true);
}
module line_sensor_bar() {
    color("black") translate([chassis_L/2-20,0,8]) cube([10,100,8],center=true);
    for (y=[-36,-18,0,18,36])
        color("lime") translate([chassis_L/2-20,y,3]) cylinder(d=7,h=3);
}
module arm_link(len=80,h=14) {
    color("goldenrod") hull() {
        translate([0,0,0]) cylinder(d=18,h=h,center=true);
        translate([len,0,0]) cylinder(d=18,h=h,center=true);
    }
}
module joint(d=28,h=18) {
    color("firebrick") cylinder(d=d,h=h,center=true);
}
module electromagnet_head() {
    // Visual placeholder only; actual magnet dimensions depend on measured SKU.
    color("dimgray") cylinder(d=magnet_d+10,h=magnet_h,center=true);
    color("silver") translate([0,0,-magnet_h/2-1.5])
        cylinder(d=magnet_d,h=3,center=true);
    // Small steel target plate shown beneath the magnet face.
    color("gold") translate([0,0,-magnet_h/2-5])
        cube([22,22,2],center=true);
}
module magnetic_arm() {
    z0 = wheel_D/2+chassis_T+arm_base_z;
    color("silver") translate([arm_base_x,0,wheel_D/2+chassis_T+16])
        cylinder(d=58,h=12);
    color("firebrick") translate([arm_base_x,0,z0])
        cylinder(d=42,h=24,center=true);
    translate([arm_base_x,0,z0+10]) {
        joint(30,22);
        translate([arm_link_1/2,0,-12]) rotate([0,-18,0])
            arm_link(arm_link_1);
        translate([arm_link_1*cos(18),0,-arm_link_1*sin(18)-12]) {
            joint(25,20);
            translate([arm_link_2/2,0,0]) rotate([0,22,0])
                arm_link(arm_link_2);
            translate([arm_link_2*cos(22),0,arm_link_2*sin(22)]) {
                joint(20,16);
                translate([20,0,0]) electromagnet_head();
            }
        }
    }
}
module robot() {
    chassis();
    wheel(-20,-wheel_track/2);
    wheel(-20,wheel_track/2);
    caster();
    line_sensor_bar();
    magnetic_arm();
}
module flatbed_cart_scene() {
    // Separate remote-controlled cart: simplified flat platform, wheels and drive pods.
    // It is scene geometry, not included in robot-only STL export.
    color([0.1,0.55,0.32,0.88])
        translate([-205,0,58]) cube([150,125,8],center=true);
    // low edge strips / anti-slip rim
    color([0.1,0.35,0.2,0.9])
        translate([-205,0,64]) difference() {
            cube([150,125,5],center=true);
            cube([140,115,7],center=true);
        }
    for (x=[-260,-150]) for (y=[-68,68])
        color("dimgray") translate([x,y,31])
            rotate([90,0,0]) cylinder(d=34,h=16,center=true);
    // motor/drive and battery/receiver placeholders under the platform
    color("royalblue") translate([-205,-28,42]) cube([54,38,24],center=true);
    color("orange") translate([-205,30,42]) cube([52,34,20],center=true);
    color("black") translate([-205,0,68]) cube([34,24,8],center=true);
}
module display_scene() {
    // Fixed product stand at the right, with a small steel target plate on the item.
    color([0.2,0.5,0.9,0.22]) translate([210,0,65])
        cube([18,190,130],center=true);
    color([0.2,0.5,0.9,0.22]) translate([198,0,125])
        cube([42,190,12],center=true);
    color("goldenrod") translate([195,0,146])
        cube([38,28,34],center=true);
    color("silver") translate([174,0,146])
        cube([2,20,20],center=true);
}
if (show_robot) robot();
if (show_scene) {
    display_scene();
    flatbed_cart_scene();
}

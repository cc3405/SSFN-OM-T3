// OMER independent autonomous shopping robot - conceptual 3D assembly
// Units: millimetres. Not a manufacturing drawing; verify all dimensions and loads.
$fn = 36;

// Main adjustable parameters
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
show_scene = true;       // set false for robot-only export
show_robot = true;
show_sensor_track = true;

// Colours are only for on-screen assembly preview.
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
    translate([x,y,wheel_D/2+chassis_T]) rotate([90,0,0])
        for (a=[0:30:330]) rotate([0,0,a]) translate([wheel_D/2-2,0,0])
            cube([3,wheel_W,2],center=true);
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
    // battery and controller enclosures
    color("royalblue") translate([-35,0,wheel_D/2+chassis_T+15])
        cube([72,76,26],center=true);
    color("darkorange") translate([-chassis_L/2+42,0,wheel_D/2+chassis_T+12])
        cube([48,54,20],center=true);
    // arm pedestal mounting plate
    color("silver") translate([arm_base_x,0,wheel_D/2+chassis_T+5])
        cylinder(d=74,h=10,center=true);
}
module line_sensor_bar() {
    color("black") translate([chassis_L/2-20,0,8]) cube([10,100,8],center=true);
    for (y=[-36,-18,0,18,36])
        color("lime") translate([chassis_L/2-20,y,3]) cylinder(d=7,h=3);
}
module arm_link(len=80,w=18,h=14) {
    color("goldenrod") hull() {
        translate([0,0,0]) cylinder(d=18,h=h,center=true);
        translate([len,0,0]) cylinder(d=18,h=h,center=true);
    }
}
module joint(d=28,h=18) {
    color("firebrick") cylinder(d=d,h=h,center=true);
}
module gripper() {
    // simplified two-finger servo gripper, open pose
    color("dimgray") cube([30,34,12],center=true);
    color("silver") translate([17,0,-2]) cube([8,10,26],center=true);
    color("silver") translate([32,-12,-2]) cube([34,7,9],center=true);
    color("silver") translate([32,12,-2]) cube([34,7,9],center=true);
    color("black") translate([49,-12,-2]) cube([5,8,10],center=true);
    color("black") translate([49,12,-2]) cube([5,8,10],center=true);
}
module robotic_arm() {
    z0 = wheel_D/2+chassis_T+arm_base_z;
    // yaw turntable
    color("silver") translate([arm_base_x,0,wheel_D/2+chassis_T+16]) cylinder(d=58,h=12);
    color("firebrick") translate([arm_base_x,0,z0]) cylinder(d=42,h=24,center=true);
    // Shoulder and elbow are shown in a reachable, forward/downward pick pose.
    translate([arm_base_x,0,z0+10]) rotate([0,0,0]) {
        joint(30,22);
        translate([arm_link_1/2,0,-12]) rotate([0,-18,0]) arm_link(arm_link_1);
        translate([arm_link_1*cos(18),0,-arm_link_1*sin(18)-12]) {
            joint(25,20);
            translate([arm_link_2/2,0,0]) rotate([0,22,0]) arm_link(arm_link_2);
            translate([arm_link_2*cos(22),0,arm_link_2*sin(22)]) {
                joint(20,16);
                translate([20,0,0]) rotate([0,0,0]) gripper();
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
    robotic_arm();
}
module shelf_scene() {
    // translucent context geometry only; not part of a recommended print.
    color([0.2,0.5,0.9,0.22]) translate([210,0,65]) cube([18,190,130],center=true);
    color([0.2,0.5,0.9,0.22]) translate([198,0,125]) cube([42,190,12],center=true);
    color([0.9,0.65,0.15,0.75]) translate([195,0,146]) cube([38,28,34],center=true); // sample item
}
module cart_scene() {
    // conceptual shopping cart located behind the robot; fit to actual project cart.
    color([0.1,0.6,0.35,0.25]) translate([-205,0,62]) cube([100,120,8],center=true);
    color([0.1,0.6,0.35,0.25]) translate([-205,0,91]) difference() {
        cube([100,120,58],center=true);
        translate([0,0,8]) cube([90,110,55],center=true);
    }
    for (y=[-48,48]) for (x=[-242,-168])
        color("dimgray") translate([x,y,25]) rotate([90,0,0]) cylinder(d=24,h=10,center=true);
}

if (show_robot) robot();
if (show_scene) {
    shelf_scene();
    cart_scene();
}

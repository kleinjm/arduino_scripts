import processing.serial.*;
Serial myPort;
PImage logo;

int bgcolor = 0;

void setup() {
  size(1, 1);
  surface.setResizable(true);
  colorMode(HSB, 255);
  
  logo = loadImage("https://europe1.discourse-cdn.com/arduino/original/4X/6/9/2/692a51906007705eba0cb142c822bc9b81a9c3b9.png");
  surface.setSize(logo.width, logo.height);
  println("Available serial ports:");
  // Type String[] of the last argument to method println(Object...) doesn't exactly match the vararg parameter type. Cast to Object[] to confirm the non-varargs invocation, or pass individual arguments of type Object for a varargs invocation.
  println(Serial.list());
  
  myPort = new Serial(this, Serial.list()[0], 9600);
}

void draw() {
  if(myPort.available() > 0){
    bgcolor = myPort.read();
    println(bgcolor);
  }
  background(bgcolor, 255, 255);
  image(logo, 0, 0);
}

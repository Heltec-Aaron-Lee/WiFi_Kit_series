function bytesToInt(by) {
    f = by[0] | by[1]<<8 | by[2]<<16 | by[3]<<24;
    return f;
} 
  
function bytesToFloat(by) {
    var bits = by[3]<<24 | by[2]<<16 | by[1]<<8 | by[0];
    var sign = (bits>>>31 === 0) ? 1.0 : -1.0;
    var e = bits>>>23 & 0xff;
    var m = (e === 0) ? (bits & 0x7fffff)<<1 : (bits & 0x7fffff) | 0x800000;
    var f = sign * m * Math.pow(2, e - 150);
    return f;
} 
  
function Decoder(bytes, port) {
  
    var decoded = {};
    i = 0;
  
    decoded.latitude = bytesToFloat(bytes.slice(i,i+=4));
    decoded.longitude = bytesToFloat(bytes.slice(i,i+=4));
    decoded.altitude = bytesToFloat(bytes.slice(i,i+=4));
    decoded.course = bytesToFloat(bytes.slice(i,i+=4));
    decoded.speed = bytesToFloat(bytes.slice(i,i+=4));
    decoded.hdop = bytesToFloat(bytes.slice(i,i+=4));
  
    decoded.battery = ((bytes[i++] << 8) | bytes[i++]);

    return decoded;
}
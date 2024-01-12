function bytesTouInt8(by) {
    f = by[0] | by[1]<<8 ;
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
  
    decoded.hour = bytesToInt(bytes.slice(i,i+=2));
    decoded.minute = bytesToInt(bytes.slice(i,i+=2));
    decoded.second = bytesToFloat(bytes.slice(i,i+=2));
    decoded.centisecond = bytesToFloat(bytes.slice(i,i+=2));
   
  
    // decoded.battery = ((bytes[i++] << 8) | bytes[i++]);

    return decoded;
}
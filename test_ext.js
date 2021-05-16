import { poll } from "./ext.so";
import * as os from "os";

const O_NONBLOCK = 4;
let fp = os.open('/dev/stdin', O_NONBLOCK);

let blen = 16;
let buf = new Uint8Array(blen);
var len;
for (let i = 0; i < 10000; i++) {
let param = {fd: fp, events:1};
let rs = poll([param], 100);
if ( rs > 0) {
    len = os.read(fp, buf.buffer, 0, blen);
}
console.log("rs:" + rs + "  " +  param.revents + "        "+ i);
}

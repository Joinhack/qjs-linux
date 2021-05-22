import { poll, fnonblock } from "./ext.so";
import * as os from "os";
import * as std from "std";

let fp = os.open('/dev/tty');
console.log("fnonblock:" + fnonblock(fp));


let blen = 1;
let buf = new Uint8Array(blen);
var len;
for (let i = 0; i < 10000; i++) {
let param = {fd: fp, events:1};
let rs = poll([param], 0);
if ( rs > 0) {
    len = os.read(fp, buf.buffer, 0, blen);
}
console.log("rs:" + rs + "  " +  param.revents + "        "+ i);
}

import * as os from "os";
import {puts} from "std";
import {PCEmulator} from "PCEmulator.js";

var pc, boot_start_time;

var binaries = [false,false,false];

function checkbinaries() {
    //console.log("checkbinaries: ",(binaries[0]!=false),(binaries[1]!=false),(binaries[2]!=false));
    if((binaries[0] != false) && (binaries[1] != false) && (binaries[2] != false)){
        console.log("...binaries done loading, calling start()")
        start();
    } else {
         os.setTimeout(checkbinaries, 500);
    }
}

function loadbinary(file, slot) {
    var offset, len, total;
    let st = os.stat(file, os.O_RDONLY);
    if (st[1] < 0)
        throw "Error while loading " + file;
    total = st[0].size;
    let buf = new ArrayBuffer(total);
    let fp = os.open(file);
    if (fp < 0) 
        throw "Error while loading " + file;
    offset = 0;
    while((len = os.read(fp, buf, offset, (total - offset)>1024?1024:(total - offset))) > 0) {
        offset += len;
    }
    binaries[slot] = buf;
}

function load_binaries() {
    console.log("requesting binaries");
    loadbinary("vmlinux-2.6.20.bin", 0);
    loadbinary("root.bin", 1);
    loadbinary("linuxstart.bin", 2);

    console.log("waiting for binaries to finish loading...");
    checkbinaries();
}

function clipboard_get() {

}

function clipboard_set() {}

function get_boot_time() {
    return (+new Date()) - boot_start_time;
}

function str2ab(str) {
  var buf = new ArrayBuffer(str.length); // 2 bytes for each char
  var bufView = new Uint8Array(buf);
  for (var i = 0, strLen = str.length; i < strLen; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  return buf;
}

function serial_write(v) {
    puts(v)
}

function start() {
    var start_addr, initrd_size, params, cmdline_addr;
    
    params = new Object();

    /* serial output chars */
    params.serial_write = serial_write;

    /* memory size (in bytes) */
    params.mem_size = 16 * 1024 * 1024;

    /* clipboard I/O */
    params.clipboard_get = clipboard_get;
    params.clipboard_set = clipboard_set;

    params.get_boot_time = get_boot_time;

    pc = new PCEmulator(params);

    pc.load_binary(binaries[0], 0x00100000);

    initrd_size = pc.load_binary(binaries[1], 0x00400000);

    start_addr = 0x10000;
    pc.load_binary(binaries[2], start_addr);

    /* set the Linux kernel command line */
    /* Note: we don't use initramfs because it is not possible to
       disable gzip decompression in this case, which would be too
       slow. */
    cmdline_addr = 0xf800;
    pc.cpu.write_string(cmdline_addr, "console=ttyS0 root=/dev/ram0 rw init=/sbin/init notsc=1");

    pc.cpu.eip = start_addr;
    pc.cpu.regs[0] = params.mem_size; /* eax */
    pc.cpu.regs[3] = initrd_size; /* ebx */
    pc.cpu.regs[1] = cmdline_addr; /* ecx */

    boot_start_time = (+new Date());

    pc.start();
}


function main() {
    load_binaries();
}

main();
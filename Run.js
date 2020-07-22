//全局变量声明
var _DEBUG_JS = true;
var _scanmodule = ['libc.so'];
// unscanmodule通常是在全部遍历排除用，一般执行如果存在scanmodule，则不关注 unscanmodule
var _unscanmodule = [];

var _copydir = '/system/etc/adlab/';

//手动拷贝
var _dexpath = '/data/local/tmp/Japi.dex';
var _elfpath = '/system/etc/adlab/libelf.so';
var _elfpath64 = '/system/etc/adlab/libelf64.so';

var _CODE_ = 'code';
var _IMPORT_ = 'import';
var _EXPORT_ = 'export';

var _MAX_CHEKC_CODE_SIZE = 0x100; //为0比较全部代码

var _IS_ARM64 = Is64();
//----------------------------Head.js
//Java.androidVersion
function getsdkint() {
    var sdkint = 0;
    const BV = Java.use('android.os.Build$VERSION');
    if(BV !== undefined) {
        const BV_instance = BV.$new();
        sdkint = BV_instance.SDK_INT.value;
        console.log("SDK_INT:"+sdkint);
    }
    return sdkint;
}


function exists(filePath) {
    var b = false;
    const File = Java.use("java.io.File");
    if(File !== undefined) {
        const File_instance = File.$new(filePath);
        b = File_instance.exists();
    }
    return b;
}


//使用命令行方式拷贝文件
function copyFile3(from, to) {
    const M = Java.use('com.jlxy.javaapi.M');
    if(M !== undefined) {
        const cmd = 'cp '+from+ ' '+ to;
        M.execCommand(cmd,true);
        return exists(to);
    }
    return false;
}


function mkdirs2(dir) {
    const M = Java.use('com.jlxy.javaapi.M');
    if(M !== undefined) {
        const cmd = 'mkdir -p '+dir;
        M.execCommand(cmd,true);
    }
}


function chmod7xx2(filePath) {
    const M = Java.use('com.jlxy.javaapi.M');
    if(M !== undefined) {
        const cmd = 'chmod 755 '+filePath;
        M.execCommand(cmd,true);
    }
}


function insertStr(soure, start, newStr){
   return soure.slice(0, start) + newStr + soure.slice(start);
}


function _struct_diff() {
    this.begin = 0;
    this.size  = 0;
}

function codememcmp(a, b , size) {
    var bl = false;
    const _sd = new _struct_diff();
    for (var i = 0; i < size; i++) {
        if(bl){
            if(a[i]  == b[i]){
                bl = false;
                 _sd.size = i - _sd.begin;
                 break;
            }
        }else{
            if(a[i]  != b[i]){
                bl = true;
                _sd.begin = i;
            }
        }
    }

    return _sd;
}

//生成python可识别的数组
function mkarray(ptr,length) {
    var array = new Array(length);
    for (var i = 0; i < length; i++) {
        const newptr = ptr.add(i);
        array[i] = newptr.readU8();
    }
    return array;
}

function debuglog(s) {
    if(_DEBUG_JS)
        console.log(s);
}
//----------------------------


/**
 * @return {boolean}
 */
function Is64() {
    return (Process.arch ==="arm64" || Process.arch ==="x64");
}

function SetScanModule(){
    if(getsdkint() < 21){
        _scanmodule.push('libdvm.so');
    }else {
        _scanmodule.push('libart.so');
    }
    if(_IS_ARM64){
         _scanmodule.push('linker64');
    }else {
         _scanmodule.push('linker');
    }
}

function arm64to(Addr, Arr) {
    /*
.text:000000000001DF9C 50 00 00 58                             LDR             X16, flt_1DFA4
.text:000000000001DFA0 00 02 1F D6                             BR              X16
.text:000000000001DFA0                         ; ---------------------------------------------------------------------------
.text:000000000001DFA4 00 53 48 A9 7F 00 00 00
     */
    var ret = "";
    //可以不是x16，为了不影响堆栈，才用的，考虑到pattern的问题，首字符不能为??，本次不智能计算了
    //0x58000050 & 0x1F ==16 = x16寄存器，再往上是跳转偏移
    const pattern = "50 00 00 58 00 02 1F D6";
    const scanSync = Memory.scanSync(Addr, Arr.length, pattern);
    if(scanSync.length > 0) {
        for (var i = 0; i < scanSync.length; i++) {
            const address = scanSync[i].address;
            const size = scanSync[i].size;

            const to = address.add(8);//不智能计算，粗糙的写法
            const m  = to.readU64();
            const jmptoModule = Process.findModuleByAddress(m);
            if(jmptoModule == null){
                ret = "Arm JmpTo addr: 0x"+m.toString(16);
            }else {
                ret = "Arm JmpTo addr: 0x"+m.toString(16) + " JmpTo path:"+jmptoModule.path;
            }
            break;
        }
    }
    return ret;
}

function armto(Addr,Arr) {
    var ret = "";
    const pattern = "0? F? ?F E5"; //首字符2个??会导致无效参数，没有办法写为0?
    const scanSync = Memory.scanSync(Addr, Arr.length, pattern);
    if(scanSync.length > 0){
        for (var i = 0; i < scanSync.length; i++) {
            const address = scanSync[i].address;
            const size = scanSync[i].size;

            const ldr = address.add(2).readU8();
            if(ldr != 0x1f && ldr != 0x9f){
                continue;
            }
            //取向前跳还是向后跳，计算逻辑是凭记忆观察写的，arm指令白皮书找不到了。
            const zf = ldr >> 7; //最小只能读8bit
            const offset  = (address.readU16()) & 0x0FFF;
            const pc = address.add(8);

            var to;
            if(zf == 1){ //向后跳转
                to = pc.add(offset);
            }else {
                to = pc.sub(offset);
            }

            const m  = to.readU32();
            const jmptoModule = Process.findModuleByAddress(m);
            if(jmptoModule == null){
                ret = "Arm JmpTo addr: 0x"+m.toString(16);
            }else {
                ret = "Arm JmpTo addr: 0x"+m.toString(16) + " JmpTo path:"+jmptoModule.path;
            }
            break;
        }
    }
    return ret;
}

function thumbto(Addr, Arr) {
    //DF F8 00 F0 ,
    // df ldr.w  d高位为1，向后跳，5高位为0，向前跳
    // f8标识thumb2指令集，因为thumb2字节太短了，不能长跳
    // 00 ?0 偏移
    // F? 寄存器值，f就是pc
    var ret = "";
    const pattern = "?F F8 ?? F?";
    const scanSync = Memory.scanSync(Addr.and(~1), Arr.length, pattern);
    if(scanSync.length > 0){
        for (var i = 0; i < scanSync.length; i++) {
            const address = scanSync[i].address;
            const size = scanSync[i].size;

            const ldr = address.readU8();
            if(ldr != 0xdf && ldr != 0x5f){
                continue;
            }
            const zf = ldr >> 7;
            const offset = (address.add(2).readU16()) & 0x0FFF;
            const pc = address.add(4);

            var to;
            if(zf == 1){
                to = pc.add(offset);
            }else {
                to = pc.sub(offset);
            }

            const m  = to.readU32();
            const jmptoModule = Process.findModuleByAddress(m);
            if(jmptoModule == null){
                ret = "Thumb JmpTo addr: 0x"+m.toString(16);
            }else {
                ret = "Thumb JmpTo addr: 0x"+m.toString(16) + " JmpTo path:"+jmptoModule.path;
            }
            break; //只显示第一个？
        }
    }
    return ret;
}

function getJmptoWhere(type,Addr,Arr) {
    var str = "";
    if(type === _CODE_){
        if(_IS_ARM64){
            str = arm64to(Addr, Arr);
        }else {
            //arm
            if(Addr.and(1).equals(0)){
                str = armto(Addr, Arr);
            }else {
                str = thumbto(Addr, Arr);
            }
        }
    }
    return str;
}

function report(libname,funcname,offset,type,Addr,Arr,trueAddr,trueArr) {
    var send_data = {};
    send_data.libname = libname;
    send_data.funcname = funcname;
    send_data.type = type;
    send_data.offset = offset;
    send_data.Addr = Addr;
    send_data.Arr = Arr;
    send_data.trueAddr = trueAddr;
    send_data.trueArr = trueArr;
    send_data.Jmpto = getJmptoWhere(type,Addr,Arr);
    send(send_data);
}

function renamelib(name) {
    //return insertStr(name, name.length - 3,"_adlab");
    return name;
}

function loadlib(path, dir, despath) {
    //是否存在
    if(exists(despath)){
        debuglog(despath+" already exists!");
    }else {
        mkdirs2(dir);
        //拷贝
        const ok = copyFile3(path,despath);
        if(ok){
            //修改属性
            chmod7xx2(despath);
        }else {
            return null;
        }

    }
    //Module.load 是通过dlopen实现，对路径有限制，拷贝libc至允许加载路径也会加载失败。
    //调整为用java代码加载一个自实现so，调用c接口mmap方式获取elf信息。
    return Module.load(despath);
}

/**
 * @return {boolean}
 */
function InitCMoudle(Hxlib) {
    const M = Java.use('com.jlxy.javaapi.C');
    if(M !== undefined) {
        return M.InitCMoudle(Hxlib);
    }
    return false;
}

function mapload(systemlib) {
    const C = Java.use('com.jlxy.javaapi.C');
    if(C !== undefined) {
        return C.mapload(systemlib);
    }
    return 0;
}

function getImport(base, funcname) {
    const C = Java.use('com.jlxy.javaapi.C');
    if(C !== undefined) {
        return C.getImport(base, funcname);
    }
    return 0;
}

function getExport(base, funcname) {
    const C = Java.use('com.jlxy.javaapi.C');
    if(C !== undefined) {
        return C.getExport(base, funcname);
    }
    return 0;
}

function getCode(base, funcname) {
    const C = Java.use('com.jlxy.javaapi.C');
    if(C !== undefined) {
        return C.getCode(base, funcname);
    }
    return 0;
}

function loadlib2(path) {
    const hxlib = _IS_ARM64 ? _elfpath64 : _elfpath;
    if(!InitCMoudle(hxlib)){
        debuglog("load failed:"+hxlib);
        return 0;
    }
   return mapload(path);
}

function readVoidPtr(ptr) {
    return _IS_ARM64 ? ptr.readU64().toNumber():ptr.readU32();
}

function checkImport(base, process_Obj_Module) {
    debuglog('begin check Imports');
    //导入表
    const IElen = _IS_ARM64? 8: 4;
    const Imports       = process_Obj_Module.enumerateImports();
    for(var i = 0; i < Imports.length; i++) {
        if(Imports[i].type !== "function"){
            continue;
        }
        const findaddr = Module.findExportByName(Imports[i].module, Imports[i].name);
        if(findaddr == null){
            continue;
        }
        const offset       = Imports[i].address; //NativePointer
        if(!offset.equals(findaddr)){
            //记录
            report(process_Obj_Module.path,
                        Imports[i].name,
                        0,
                        _IMPORT_,
                        offset,
                        [],
                        findaddr,
                        []
                    );
        }
    }
}

function checkExport(base, process_Obj_Module) {
    debuglog('begin check Exports');
    //导出表
    const Exports       = process_Obj_Module.enumerateExports();
    for(var i = 0; i < Exports.length; i++) {
        if(Exports[i].type !== "function"){
            continue;
        }
        const addrptr      = Exports[i].address;
        const mapaddr   = getExport(base, Exports[i].name);
        if(mapaddr == 0){
            continue;
        }
        const mapaddrptr = new NativePointer(mapaddr);

        //const offset     = addrptr.sub(process_Obj_Module.base); //不能用这句，没有load_bias
        const tmp = process_Obj_Module.base.add(mapaddrptr.sub(base));
        //readPointer返回值还是NativePointer，不好比较
        const offset = readVoidPtr(tmp);
        const map_offset = readVoidPtr(mapaddrptr);

        if(offset != map_offset){
            //记录
            report(process_Obj_Module.path,
                        Exports[i].name,
                        0,
                        _EXPORT_,
                        tmp,
                        mkarray(tmp,IElen),
                        mapaddrptr,
                        mkarray(mapaddrptr,IElen)
                    );
        }
    }
}

function checkCode(base, process_Obj_Module) {
    debuglog('begin check Code');
    //代码段
    const Symbol       = process_Obj_Module.enumerateSymbols();
    for(var i = 0; i < Symbol.length; i++) {
        if(Symbol[i].type !== "function"){
            continue;
        }
        if(Symbol[i].address.equals(0)){
            continue;
        }
        if(!Symbol[i].isGlobal){
            continue;
        }

        var len  = Symbol[i].size;
        if(_MAX_CHEKC_CODE_SIZE != 0 && len > _MAX_CHEKC_CODE_SIZE){ //函数太长不比较了，容易超时
            continue;
        }

        const mapaddr   = getCode(base, Symbol[i].name);
        if(mapaddr == 0){
            continue;
        }

        var addr;
        if(_IS_ARM64){
            addr = Symbol[i].address;
        }else {
            addr = Symbol[i].address.and( ~1);
        }
        var l    = 0;
        const toNptr = new NativePointer(mapaddr);
        var mapaddrptr;
         if(_IS_ARM64){
            mapaddrptr = toNptr;
        }else {
            mapaddrptr = toNptr.and(~1);
        }
        do {
            //每个不同的区间分别报告
            const addr_l     = addr.add(l);
            const map_addr_l = mapaddrptr.add(l);
            const xlen       = len - l;

            const a_arr = mkarray(addr_l,xlen);
            const b_arr = mkarray(map_addr_l,xlen);
            const _sd = codememcmp(a_arr, b_arr, xlen);
            if(_sd.size == 0){
                break;
            }
            l += _sd.begin;
            //debuglog(mkarray(addr,len));
            //debuglog(mkarray(mapaddrptr,len));
            //记录
            report(process_Obj_Module.path,
                Symbol[i].name,
                l,
                _CODE_,
                Symbol[i].address.add(l), //为了区分arm、thumb
                mkarray(addr.add(l),_sd.size),
                toNptr.add(l),
                mkarray(mapaddrptr.add(l),_sd.size)
            );
            l += _sd.size;
        }while (true);
    }
}

function scanonemodule(process_Obj_Module) {
    debuglog(process_Obj_Module.path);

    const base = loadlib2(process_Obj_Module.path);
    if (base === 0){
        console.log("load failed:"+despath);
        return;
    }
    checkImport(base,process_Obj_Module);
    checkExport(base, process_Obj_Module);
    checkCode(base, process_Obj_Module)
    debuglog('end check');
}

function makescanlist(process_Obj_Module_Arr) {
    if( _scanmodule.length <= 0){
        console.log("no add scanmodule");
        if( _unscanmodule.length <= 0){
            console.log("no add unscanmodule, too!");
            console.log("you want scan all, so slow!");
            for(var i = 0; i < process_Obj_Module_Arr.length; i++) {
                scanonemodule(process_Obj_Module_Arr[i]);
            }
        }
        else {
            const usmaStr = _unscanmodule.toString();
            for(var i = 0; i < process_Obj_Module_Arr.length; i++) {
            if(usmaStr.indexOf(process_Obj_Module_Arr[i].name) < 0)
                scanonemodule(process_Obj_Module_Arr[i]);
            }
        }
    }else {
        const smaStr = _scanmodule.toString();
        for(var i = 0; i < process_Obj_Module_Arr.length; i++) {
            if(smaStr.indexOf(process_Obj_Module_Arr[i].name) >= 0){
                scanonemodule(process_Obj_Module_Arr[i]);
            }
        }
    }

}

//main
Java.perform(function () {
    SetScanModule();
    Java.openClassFile(_dexpath).load();
    const process_Obj_Module_Arr = Process.enumerateModules();
    makescanlist(process_Obj_Module_Arr);
});
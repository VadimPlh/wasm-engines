function Initialize() {
}

function user_script(obj) {
    let array = new Int32Array(obj);
    array[2] = array[0] + array[1];
}

Initialize();
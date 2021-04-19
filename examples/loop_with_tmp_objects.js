function user_script(obj) {
    let it_count = new Int32Array(obj);
    let array_map = new Map();
    for (let i = 0; i < it_count; ++i) {
        let tmp_array = new ArrayBuffer(100);
        array_map[i] = tmp_array;
    }
}
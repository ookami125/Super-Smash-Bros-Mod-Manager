
//load first argument as file name
var fs = require('fs');
var file = process.argv[2];

var json = JSON.parse(fs.readFileSync(file, 'utf8'));

function sortObject(o) {
    var sorted = {},
    key, a = [];

    for (key in o) {
        if (o.hasOwnProperty(key)) {
            a.push(key);
        }
    }

    a.sort();

    for (key = 0; key < a.length; key++) {
        var temp = o[a[key]];
        if(temp instanceof Array)
            sorted[a[key]] = temp.sort();
        else if(temp instanceof Object)
            sorted[a[key]] = sortObject(temp);
        else
            sorted[a[key]] = temp;
    }
    return sorted;
}

//write to file
fs.writeFile("new-"+file, JSON.stringify(sortObject(json), null, 4), function(err) {
    if(err) {
        console.log(err);
    } else {
        console.log("JSON saved to " + file);
    }
});
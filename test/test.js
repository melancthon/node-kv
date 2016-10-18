var lmdb = require("../").lmdb
var path = require("path")
//var expect = require("expect.js")

var envPath = path.join(__dirname, 'testdb', 'lmdb')
try { require("../lib/rmdir.js")(envPath);} catch(e) { }

//describe("LMDB int32-int32", function() {
	var db_env = new lmdb.Env({
	  dir: envPath
	})
	var db = db_env.openDb({
	  name: 'ddr4', keyType: 'int32', valType: 'int32'
	})
	//db.put("__initKey", Math.random())
	//console.log(db.get("__yo"))
	//db.close()

//	after(function() {
		db_env.close()
//	})
//})

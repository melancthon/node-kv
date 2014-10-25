var path = require('path'),
    expect = require('expect.js'),
    lmdb = require('../').lmdb;

var envPath = path.join(__dirname, 'testdb');
try { require('../lib/rmdir.js')(envPath); } catch (e) { }

describe('LMDB int32-int32', function () {
    var env = new lmdb.Env({
        dir: envPath
    });

    var db = env.openDb({
        name: 'test',
        keyType: 'int32',
        valType: 'int32'
    });

    var dupdb = env.openDb({
        name: 'testdup',
        keyType: 'int32',
        valType: 'hex',
        allowDup: true
    });


    it('should throw exception when open db with same name.', function () {
        expect(function () {
            env.openDb({ name: 'test' });
        }).to.throwException();
    });

    it('should work as expected.', function () {
        db.put(1, 1);
        expect(db.get(1)).to.be(1);
        expect(db.del(1)).to.be(true);
        expect(db.get(1)).to.be(null);
        expect(db.del(1)).to.be(false);
    });

    it('batch should work as expected.', function (fcb) {
        db.batchPut(6, 6);
        db.batchPut(7, 7);
        db.batchDel(7);
        expect(db.get(6)).to.be(null);
        expect(db.get(7)).to.be(null);
        env.flushBatchOps();
        expect(db.get(6)).to.be(6);
        expect(db.get(7)).to.be(null);
        db.batchPut(7, 7);
        setTimeout(function () {
            expect(db.get(7)).to.be(7);
            fcb();
        }, 50);
    });

    it('txn should work as expected.', function () {
        try { db.del(2); } catch (e) { }

        var txn = env.beginTxn();
        db.put(2, 2, txn);
        expect(db.get(2)).to.be(null);
        expect(db.get(2, txn)).to.be(2);
        txn.abort();
        expect(db.get(2)).to.be(null);

        txn = env.beginTxn();
        db.put(2, 2, txn);
        expect(db.get(2)).to.be(null);
        expect(db.get(2, txn)).to.be(2);
        txn.commit();
        expect(db.get(2)).to.be(2);
    });

    it('dup should work as expected.', function () {
        dupdb.put(1, '1adb');
        expect(dupdb.exists(1, '1adb')).to.be(true);
        expect(dupdb.exists(1, '2adb')).to.be(false);
        dupdb.put(1, '2adb');
        expect(dupdb.exists(1, '1adb')).to.be(true);
        expect(dupdb.exists(1, '2adb')).to.be(true);

        var txn = env.beginTxn();
        dupdb.put(2, '1adb', txn);
        expect(dupdb.exists(2, '1adb', txn)).to.be(true);
        expect(dupdb.exists(2, '1adb')).to.be(false);
        dupdb.put(2, '2adb', txn);
        expect(dupdb.exists(2, '2adb', txn)).to.be(true);
        expect(dupdb.exists(2, '2adb')).to.be(false);
        txn.commit();
        expect(dupdb.exists(2, '1adb')).to.be(true);
        expect(dupdb.exists(2, '2adb')).to.be(true);
    });

    it('cursor should work as expected.', function () {
        var i, j;
        for (i = 100; i < 110; i++) {
            db.put(i, i);
            for (j = 0; j < 10; j += 3) {
                dupdb.put(i, j + 'abc');
                dupdb.put(i, j + 'abc');
            }
        }

        var txn = env.beginTxn();
        var cur = db.cursor(txn), dupcur = dupdb.cursor(txn);
        expect(function () { cur.current(); }).to.throwException();
        expect(function () { cur.seek(100, 100); }).to.throwException();
        for (i = 100; i < 101; i++) {
            expect(cur.seek(i)).to.eql([i, i]);
            for (j = 0; j < 10; j++) {
                if (0 === j % 3) expect(dupcur.seek(i, j + 'abc')).to.eql([i, j + 'abc']);
                else expect(dupcur.seek(i, j + 'abc')).to.be(null);
            }
        }

        expect(cur.seek(99)).to.be(null);
        expect(cur.lowerBound(99)).to.eql([100, 100]);
        expect(cur.lowerBound(106)).to.eql([106, 106]);
        expect(cur.seek(111)).to.be(null);
        expect(dupcur.lowerBound(99)).to.eql([100, '0abc']);
        expect(dupcur.lowerBound(99, '000')).to.be(null);
        expect(dupcur.lowerBound(100, '0abc')).to.eql([100, '0abc']);
        expect(dupcur.lowerBound(100, '1abc')).to.eql([100, '3abc']);
        expect(dupcur.lowerBound(100, '2abc')).to.eql([100, '3abc']);
        expect(dupcur.lowerBound(100, '3abc')).to.eql([100, '3abc']);

        expect(dupcur.seek(100)).to.eql([100, '0abc']);
        expect(dupcur.nextDup()).to.eql([100, '3abc']);
        expect(dupcur.nextDup()).to.eql([100, '6abc']);
        expect(dupcur.nextDup()).to.eql([100, '9abc']);
        expect(dupcur.nextDup()).to.be(null);

        expect(dupcur.seek(100)).to.eql([100, '0abc']);
        expect(dupcur.next()).to.eql([100, '3abc']);
        expect(dupcur.next()).to.eql([100, '6abc']);
        expect(dupcur.next()).to.eql([100, '9abc']);
        expect(dupcur.next()).to.eql([101, '0abc']);

        cur.close();
        dupcur.close();
        txn.commit();
    });

    after(function () {
        env.close();
    });
});
var myvar = 10;

function go()
{
    var a = Qt.include("http://127.0.0.1:8111/remote_file.js",
                       function(o) {
                            test2 = o.status == o.OK
                            test3 = a.status == a.OK
                            test4 = myvar == 13

                            done = true;
                       });
    test1 = a.status == a.LOADING


    var b = Qt.include("http://127.0.0.1:8111/exception.js",
                       function(o) {
                            test7 = o.status == o.EXCEPTION
                            test8 = b.status == a.EXCEPTION
                            test9 = b.exception.toString() == "Whoops!";
                            test10 = o.exception.toString() == "Whoops!";

                            done2 = true;
                       });
    test6 = b.status == b.LOADING
}

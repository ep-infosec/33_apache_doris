SELECT count() FROM hits WHERE UserID IN (SELECT UserID FROM hits WHERE CounterID = 800784)

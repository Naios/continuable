# Continuable
[![Build Status](https://travis-ci.org/Naios/Continuable.svg?branch=master)](https://travis-ci.org/Naios/Continuable)

> C++11 Continuation chains (Work in progress)

This library aims to provide full support for **async continuation chains with callbacks**.

***

**Important note:** Everything is work in progress and the library is not ready to be use yet, most showed examples are not working yet and are just there for explaining my plans.

## Creating Continuables

#### Create a continuable from a callback taking function

```c++
Continuable<std::string> continuable =
    make_continuable([](Callback<std::string>&& callback)
    {
	    callback("some data");
    });

```

#### Providing helper functions

```c++
Continuable<ResultSet> mysql_query(std::string&& query)
{
    return make_continuable([query = std::move(query)](Callback<ResultSet>&& callback) mutable
    {
    	// Pass the callback to the handler
        // which calls the callback when finished.
        // Everything which takes a callback works with continuables.
	    mysql_handle_async_query(std::move(query), std::move(callback));
    });
}

// You can use the helper function like you would normally do:
mysql_query("DELETE FROM users WHERE id = 27361");

// Or using chaining to handle the result which is covered in the next topic .
mysql_query("SELECT id, name FROM users")
	.then([](ResultSet result)
    {
    });
```

## Chaining Continuables

Chaining continuables is very easy:

```c++
(mysql_query("SELECT id, name FROM users")
	&& http_request("http://github.com"))
    .then([](ResultSet result, std::string page_content)
    {
    	// Pass one argument to the next continuation...
    	return page_content.empty();

        // ... or pass multiple args using tuples...
        return std::make_tuple(std::move(result), page_content.empty());

        // ... return the next continuable to process
        return mysql_query("SELECT id name FROM sessions");
    });
```

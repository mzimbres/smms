## Description

A very simple single-threaded HTTP(S) server to `POST` and `GET` files from. The url for `POST`'ing (uploading) a file looks like this

```
http://example.com/dir1/dir2/dir3/.../filename.jpg?hmac=value
```

where `/dir1/dir2/dir3/...` is the directory where the file should be stored, if the directory does not exist it is created. The value of the `hmac` query field is the hex string representation the hmac-sha-256 of `HTTP` target `/dir1/dir2/dir3/.../filename.jpg`.

The same url used to upload the file can be also used to `GET` it. In the example above that would be 

```
http://example.com/dir1/dir2/dir3/.../filename.jpg
```

# Usage

There are many possible usages for this server, for example, if you want to allow users to upload images to a server where a portal issues the HMAC in a controlled way. By carefully choosing the directories one can fill a disk uniformly.

# Example

To setup the server first generate a key that is only known by the server and the url-generating portal

```bash
$ smms-keygen --make-key
275504e306f0977f8bf9298102d1a62a97bf3faa7cb5c491a0f215b670c494fd
```

then generate the HTTP target where you will post the file.

```bash
$ smms-keygen --make-hmac /dir1/dir2/dir3/filename.jpg --key 275504e306f0977f8bf9298102d1a62a97bf3faa7cb5c491a0f215b670c494fd
e70959a5210d8e60685005197e54b556351b5f4a85cef3cedc4ce9ef5f1d0e89
```

upload the image. For example

```bash
curl -H 'Expect:' -v --data-binary @image.jpg http://host.com/dir1/dir2/dir3/filename.jpg?hmac=e70959a5210d8e60685005197e54b556351b5f4a85cef3cedc4ce9ef5f1d0e89
```

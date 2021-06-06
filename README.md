## Description

A very simple single-threaded HTTP server to `POST` and `GET` files from. The url for `POST`'ing (uploading) a file looks like this

```
http://example.com/x/y/z/.../filename.jpg?hmac=digest
```

where `/x/y/z/...` is the directory where the file should be stored, if the directory does not exist it is created. The `digest` part of the `url` is the hex string obtained by hashing the `HTTP` target `/x/y/z/.../filename.jpg` with `crypto_generichash` from `libsodium` with a secret key.

The same url used to upload the file can be also used to `GET` it. In the example above that would be 

```
http://example.com/x/y/z/.../filename.jpg
```

# Usage

There are many possible usages for this server, for example, if you want to allow users to upload images to a server where a portal issues the HMAC in a controlled way. By carefully choosing the directories one can fill a disk uniformly.

# Example

To setup the server first generate a key that is known only by the server and the url-generating portal

```bash
$ smms-keygen --make-key
275504e306f0977f8bf9298102d1a62a97bf3faa7cb5c491a0f215b670c494fd
```

them generate the HTTP target where you will post the file.

```bash
$ smms-keygen --make-hmac /dir1/dir2/dir3/filename.jpg --key 275504e306f0977f8bf9298102d1a62a97bf3faa7cb5c491a0f215b670c494fd
e70959a5210d8e60685005197e54b556351b5f4a85cef3cedc4ce9ef5f1d0e89
```

upload the image. For example

```bash
curl -H 'Expect:' -v --data-binary @image.jpg http://host.com/dir1/dir2/dir3/filename.jpg?hmac=e70959a5210d8e60685005197e54b556351b5f4a85cef3cedc4ce9ef5f1d0e89
```

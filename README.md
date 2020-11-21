## Description

A very simple single-threaded HTTP server to `POST` and `GET` files from. The url for `POST`'ing (uploading) a file looks like this

```
http://example.com/x/y/z/.../filename-digest.jpg
```

where `/x/y/z/...` is the directory where the file should be stored, if the directory does not exist it is created. The `digest` part of the `url` is the hex string obtained by hashing `/x/y/z/.../filename` with `crypto_generichash` from `libsodium`.

The same url used to upload the file can be also used to `GET` it.

# Usage

There are many possible usages for this server, for example, if you want to allow users to upload images to a server where a portal issues the signed url's in a controlled way. By carefully choosing the directories one can fill a disk uniformly.

# Example

To setup the server first generate a key that is known only by the server and the url-generating portal

```bash
$ smms-keygen --make-key
7lym0v3dqa221pz6wru0jad37z1ccclc
```

Generate the HTTP target, don't add the file extension at this point

```bash
$ smms-keygen --make-http-target /dir1/dir2/dir3/filename --key 7lym0v3dqa221pz6wru0jad37z1ccclc
/dir1/dir2/dir3/filename-e77adb77dcc6c9fabeb4674816106f7f
```

Add the extension (optional) and upload the image

```bash
curl -H 'Expect:' --data-binary @image.jpg http://host.com/dir1/dir2/dir3/filename-e77adb77dcc6c9fabeb4674816106f7f.jpg -v
```


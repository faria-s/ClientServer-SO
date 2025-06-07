# SO

## Grade: 18.75/20 ⭐️

A Server-Client App, made for our [SO](https://www.di.uminho.pt/~jno/sitedi/uc_J304N1.html) class.

This project:
- Allows communication between a server an client, through FIFOs.
- Implements a LRU cache, using GLib and linked-lists, allowing for more efficient searches in the server
- Multi processes concurrent search, allowing for more efficient searches in the server
- Presistence of information in disk

#### Compiling
```bash
$ make
```

#### Running the server
```bash
$ ./bin/dserver <dataset_path> <cache_size>
```

#### Indexing a file
```bash
$ ./bin/dclient -a "title" "authors" "year" "path"
```

#### Deleting a file
```bash
$ ./bin/dclient -a "title" "authors" "year" "path"
```

#### Shutting server down
```bash
$ ./bin/dclient -f
```

#### Consulting a file meta_information
```bash
$ ./bin/dclient -c "key"
```

#### Searching for the number of lines that a keyword appeares in a file
```bash
$ ./bin/dclient -l "key" "keyword"
```

#### Searching for the files where a keyword appears
```bash
$ ./bin/dclient -s "keyword" "nr_processes"(optional)
```

## Contributing

As a university group project, we cannot allow external contributors.

## Group Members

* [Salomé Faria](https://github.com/faria-s/) (a108487)
* [Sara Silva](https://github.com/sarasilv-a) (a104608)
* [Zita Duarte](https://github.com/ZitaMDuarte) (a104268)

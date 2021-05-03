### Usage
The script `topogen.py` will help you to handle [The Internet Topology Zoo](http://www.topology-zoo.org/index.html) dataset.
```
wget http://www.topology-zoo.org/files/archive.zip
unzip archive.zip *.gml -d zoo
python topogen.py zoo
```
This generates the five sample topologies in `./output/`. Remember to move them to `/data/topo/` before using. It is easy to modify `topogen.py` to generate more topologies from the dataset.
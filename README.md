## Utility program to create Windows Service to launch Gemfire Server.
For more details about Gemfire [click Here](http://www.vmware.com/products/application-platform/vfabric-gemfire/overview.html) see 


##Example Usage :

 Usage :
 ****  Starting and Stopping Service  ****
 Use Windows Service Control Manager or `net start/stop` commands


 ****  Creating a new service  ****
 ```
 GemfireSvcLauncher --install --type [locator | cacheserver ] --name <Name Of the Service> --params <All the required parameters required to start cacheserver/locator process>
 ```
 
 Note: No space is allowed in service name.
 Example:
 
```
 GemfireSvcLauncher --install --type cacheserver --name <MyGemFireService> --params -J-Xmx1024m -J-Xms128m cache-xml-file=c:\cacheserver.xml -dir=E:\gfecs  mcast-port=0  log-level=config
```
 
 ![Services](https://raw.github.com/davinash/GemfireSvcLauncher/master/services.jpg)
 
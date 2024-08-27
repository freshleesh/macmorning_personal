from rosbags.rosbag1 import Reader
from rosbags.typesys import Stores, get_typestore


# Create a typestore and get the string class.
typestore = get_typestore(Stores.LATEST)

# Create reader instance and open for reading.
with Reader('/home/leesh/bag/2023-07-07-15-02-52.bag') as reader:
    # Topic and msgtype information is available on .connections list.
    for connection in reader.connections:
        print(connection.topic, connection.msgtype)

    # Iterate over messages.
    for connection, timestamp, rawdata in reader.messages():
        if connection.topic == 'velodyne_points':
            msg = typestore.deserialize_ros1(rawdata, connection.msgtype)
            print(msg.header.frame_id)

    # The .messages() method accepts connection filters.
    connections = [x for x in reader.connections if x.topic == 'velodyne_points']
    for connection, timestamp, rawdata in reader.messages(connections=connections ):
        msg = typestore.deserialize_ros1(rawdata, connection.msgtype)
        print(msg.header.frame_id)
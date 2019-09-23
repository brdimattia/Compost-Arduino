import serial
import boto3
import json
from datetime import datetime
import calendar
import uuid
from base64 import b64encode

STREAM_NAME = 'iot-hackathon-compost-monster'
SERIAL_PORT = '/dev/ttyACM0'
SERIAL_RATE = 115200

ser = serial.Serial(SERIAL_PORT, SERIAL_RATE)
kinesis_client = boto3.client("firehose", region_name="us-east-1")

def put_to_stream(guid, value, timestamp):
    payload = {
                "guid" : str(guid),
                "value" : value,
                "timestamp" : timestamp
                }    
    put_response = kinesis_client.put_record(DeliveryStreamName=STREAM_NAME, Record={"Data" : json.dumps(payload)})
    return put_response

while True:
    if ser.in_waiting > 0:
        line = ser.readline().decode()
        print("Received from Serial: ", line)
        guid = uuid.uuid4()
        timestamp = calendar.timegm(datetime.utcnow().timetuple())
        response = put_to_stream(guid, line.rstrip(), timestamp)
        print("Put to stream %s with details %s" % (STREAM_NAME, response))
        
        

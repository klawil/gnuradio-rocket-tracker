'use client';

import { ListDevicesApi } from "@/types/DevicesApi";
import { typeFetch } from "@/utils/typeFetch";
import { useCallback, useEffect, useReducer, useState } from "react";
import Button from 'react-bootstrap/Button';
import Col from 'react-bootstrap/Col';
import Form from 'react-bootstrap/Form';
import Row from 'react-bootstrap/Row';
import Spinner from 'react-bootstrap/Spinner';
import Table from 'react-bootstrap/Table';
import styles from './page.module.css';
import 'leaflet/dist/leaflet.css';

import { CircleMarker, MapContainer } from "react-leaflet";

// Fix for default icon issues
import { deviceListReducer, DeviceState } from '@/utils/device';
import { CenterDevice, MapMarker, TileLayers, useUserLocation } from '@/utils/map';
import { useMessaging } from "@/utils/ws";
import { timeToDelta, useTime } from "@/utils/time";

const initMapCenter: [ number, number ] = [
  40.8444,
  -119.1123,
];

export default function HomePage() {
  const [ isLoading, setIsLoading ] = useState<boolean>(false);
  const [ devices, dispatchDevices ] = useReducer(deviceListReducer, []);
  const [ deviceErr, setDeviceErr ] = useState<string | null>(null);
  const [ centerDev, setCenterDev ] = useState<DeviceState | null>(null);
  const [ loadAll, setLoadAll ] = useState<boolean>(false);
  const time = useTime();

  const loadDevices = useCallback(async (loadAll: boolean = false) => {
    setIsLoading(true);
    setDeviceErr(null);
    try {
      const [ code, response ] = await typeFetch<ListDevicesApi>({
        path: '/api/devices/',
        method: 'GET',
        query: {
          all: `${loadAll}`,
        },
      });
      if (
        code !== 200 ||
        !response ||
        response.Msg !== '' ||
        !('Devices' in response)
      ) {
        console.log(response);
        throw new Error(`Code: ${code}, Response: ${response}`);
      }

      if (response.Devices !== null) {
        response.Devices.map(d => dispatchDevices({
          Type: 0,
          DeviceName: d.DeviceName,
          Serial: d.DeviceSerial,
          CombinedState: d.CombinedState,
        }))
      }
    } catch (e) {
      setDeviceErr(`${e}`);
      console.error('Error loading devices', e);
    }
    setIsLoading(false);
  }, []);

  const [ popMessage ] = useMessaging('LOCATION', 0);
  useEffect(() => {
    const message = popMessage();
    if (typeof message !== 'undefined') {
      dispatchDevices(message);
    }
  }, [ popMessage, ]);

  useEffect(() => {
    if (
      !isLoading &&
      devices.length === 0 &&
      deviceErr === null
    ) {
      loadDevices(loadAll);
    }
  }, [ isLoading, deviceErr, loadAll, devices, loadDevices ]);

  const showErr = deviceErr !== null ||
    (
      !isLoading &&
      (
        devices === null ||
        devices.length === 0
      )
    );
  const errMsg = deviceErr !== null
    ? deviceErr
    : 'No devices found';

  // const hasDevices = !isLoading && devices !== null && devices.length > 0;
  const hasGpsDevices = !isLoading && devices !== null
    && devices.find(dev => dev.Location.LastUpdate !== null && dev.Location.Locked);

  const userLoc = useUserLocation();
  const resetCenter = () => {
    setCenterDev(null);
  }

  return (
    <>
      <h1 className="text-center">Altus Rocket Tracker</h1>

      {(hasGpsDevices || userLoc !== null) && <Row className='justify-content-center mb-3'><Col md={6}>
      {/* {hasGpsDevices && <Row className='justify-content-center mb-3'><Col md={6}> */}
        <MapContainer
          className={styles.map}
          center={userLoc !== null
            ? userLoc
            : hasGpsDevices
              ? [
                hasGpsDevices.Location.Latitude || initMapCenter[0],
                hasGpsDevices.Location.Longitude || initMapCenter[1],
              ]
              : initMapCenter}
          zoom={14}
          maxZoom={20}
          // minZoom={14}
        >
          <TileLayers />
          {devices !== null && devices
            .filter(device => device.Location.Locked)
            .map(device => <MapMarker device={device} key={device.Serial}>
              <b>{ device.Config.DeviceName || '??' } ({ device.Serial })</b><br />
              <Button href={`/device?serial=${device.Serial}`}>Track</Button>
            </MapMarker>)
          }
          {userLoc !== null && <CircleMarker
            center={userLoc}
            radius={5}
            fillOpacity={1}
            opacity={0}
            fillColor='red'
            color='red'
          />}
          <CenterDevice
            center={centerDev}
            setCenter={resetCenter}
          />
        </MapContainer>
      </Col></Row>}

      <Row><Col className='justify-content-center'>
        <Form.Check
          type="checkbox"
          checked={loadAll}
          onChange={e => {
            setLoadAll(e.target.checked);
            loadDevices(e.target.checked);
          }}
          label="Load all devices"
        />
      </Col></Row>

      <Table striped={true} className="text-center">
        <thead>
          <tr>
            <th>Call</th>
            <th>Device</th>
            {/* <th>Name</th> */}
            <th>State</th>
            <th>Last Seen</th>
            <th>Freq</th>
            <th></th>
          </tr>
        </thead>
        <tbody>
          {isLoading && <tr><td className="p-4" colSpan={100}><Spinner /></td></tr>}
          {showErr && <tr><td className="pt-2 pb-2 text-danger" colSpan={100}>{ errMsg }</td></tr>}
          {!isLoading && devices.length > 0 && devices.sort((a, b) => a.Serial > b.Serial ? 1 : -1).map(device => (<tr
            key={device.Serial}
            onClick={() => setCenterDev(device)}
            className={`align-middle ${time - device.LastSeen.getTime() <= 60000 ? 'table-active' : ''}`}
          >
            <td>{ device.Config.CallSign || '-' } ({ device.Serial })</td>
            <td>{ device.Config.DeviceName || '??' } ({ device.Serial })</td>
            {/* <td>{ device.Name || 'N/A' }</td> */}
            <td>{ device.State.Name || 'UNKNOWN' }</td>
            <td>{ timeToDelta(time, device.LastSeen.getTime(), 300) }</td>
            <td>{ device.Channel.toFixed(3) }</td>
            <td><Button href={`/device?serial=${device.Serial}`}>Track</Button></td>
          </tr>))}
        </tbody>
      </Table>
    </>
  );
}

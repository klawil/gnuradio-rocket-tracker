'use client';

import { ListDevicesApi } from "@/types/DevicesApi";
import { typeFetch } from "@/utils/typeFetch";
import { useCallback, useEffect, useReducer, useState } from "react";
import { Button, Col, Container, Form, Row, Spinner, Table } from "react-bootstrap";
import styles from './page.module.css';
import 'leaflet/dist/leaflet.css';

import { CircleMarker, MapContainer } from "react-leaflet";

// Fix for default icon issues
import { deviceListReducer, DeviceState } from '@/utils/device';
import { CenterDevice, MapMarker, TileLayers, useUserLocation } from '@/utils/map';
import { useMessaging } from "@/utils/ws";

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
      devices === null &&
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
    <Container fluid={true}>
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
            .map(device => <MapMarker device={device} key={device.Serial} />)
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
            <th>Serial</th>
            <th>Name</th>
            <th>Type</th>
            <th>State</th>
            <th>Last Seen</th>
            <th></th>
          </tr>
        </thead>
        <tbody>
          {isLoading && <tr><td className="p-4" colSpan={100}><Spinner /></td></tr>}
          {showErr && <tr><td className="pt-2 pb-2 text-danger" colSpan={100}>{ errMsg }</td></tr>}
          {!isLoading && devices !== null && devices.length > 0 && devices.map(device => (<tr
            key={device.Serial}
            onClick={() => setCenterDev(device)}
            className='align-middle'
          >
            <td>{ device.Serial }</td>
            <td>{ device.Name || 'N/A' }</td>
            <td>{ device.Config.DeviceName || 'N/A' } ({ device.Config.Device || 'N/A' })</td>
            <td>{ device.State.Name || 'UNKNOWN' }</td>
            <td>{ device.LastSeen.toLocaleString() }</td>
            <td><Button href={`/device?serial=${device.Serial}`}>Track</Button></td>
          </tr>))}
        </tbody>
      </Table>
    </Container>
  );
}

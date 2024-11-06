import { AppDialogService } from './app-dialog.service';
// Post requests service

import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable, ObservableLike, catchError, throwError } from 'rxjs';
import { HttpHeaders } from '@angular/common/http';
import { API_BASE_URL } from '../../api_addresses';

@Injectable({
  providedIn: 'root'
})

export class PostRequestsService {
  constructor(private httpClient: HttpClient, private appDialogService: AppDialogService) { }
  serverUrl: string = API_BASE_URL;

  sendPairBLEThreadCommand(deviceId: string, pinCode: string, dataset: string, discriminator: string) : Observable<string> {
    const data = {
      nodeId: deviceId,
      pinCode: pinCode,
      type: "ble-thread",
      nodeAlias: "No Value",
      dataset: dataset,
      discriminator: discriminator
    };

    console.log(
      `Sending pairing request to ${this.serverUrl}/pairing with data: ${JSON.stringify(data)}`
    );

    return this.httpClient.post<string>(
      `${this.serverUrl}/pairing`, JSON.stringify(data)).pipe(
      catchError((error: any) => {
        console.error('Error sending pairing request', error);
        return throwError(error.message)
      }
    ));
  }

  sendPairBLEWifiCommand(deviceId: string, pinCode: string, net_ssid: string, net_pass: string, discriminator: string) : Observable<string> {
    const data = {
      nodeId: deviceId,
      pinCode: pinCode,
      type: "ble-wifi",
      nodeAlias: "No Value",
      ssId: net_ssid,
      password: net_pass,
      discriminator: discriminator
    };
    console.log(
      `Sending pairing request to ${this.serverUrl}/pairing with data: ${JSON.stringify(data)}`
    )
    // Send the pairing request to the server
    return this.httpClient.post<string>(
      `${this.serverUrl}/pairing`, JSON.stringify(data)).pipe(
      catchError((error: any) => {
        console.error('Error sending pairing request', error);
        return throwError(error.message)
      }
    ));
  }

  sendPairOnNetworkCommand(deviceId: string, passCode: string) : Observable<string>{
    const data = {
      nodeId: deviceId,
      passCode: passCode,
      type: "onnetwork",
      nodeAlias: "No Value"
    };

    console.log(
      `Sending pairing request to ${this.serverUrl}/pairing with data: ${JSON.stringify(data)}`
    )

    return this.httpClient.post<string>(
      `${this.serverUrl}/pairing`, JSON.stringify(data)
    ).pipe(
      catchError((error: any) => {
        console.error('Error sending pairing request', error);
        return throwError(error.message)
      }
    ));
  }

  sendPairEthernetCommand(deviceId: string, pinCode: string, discriminator: string, ip_address: string, port: string) : Observable<string> {
    const data = {
      nodeId: deviceId,
      pinCode: pinCode,
      type: "ethernet",
      discriminator: discriminator,
      ip_address: ip_address,
      port: port,
      nodeAlias: "No Value"
    };
    console.log(
      `Sending pairing request to ${this.serverUrl}/pairing with data: ${JSON.stringify(data)}`
    );

    return this.httpClient.post<string>(
      `${this.serverUrl}/pairing`, JSON.stringify(data)).pipe(
      catchError((error: any) => {
        console.error('Error sending pairing request', error);
        return throwError(error.message)
      }
    ));

  }

  sendOnOffToggleEndpointCommand(deviceId: string, endPointId: string, type: string) : Observable<string> {
    const data = {
      nodeAlias: "No Value",
      nodeId: deviceId,
      endPointId: endPointId,
      type: type
    };
    console.log(
      `Sending on/off toggle request to ${this.serverUrl}/onoff with data: ${JSON.stringify(data)}`
    );

    return this.httpClient.post<string>(
      `${this.serverUrl}/onoff`, JSON.stringify(data)).pipe(
      catchError((error: any) => {
        console.error('Error sending on/off toggle request', error);
        return throwError(error.message)
      }
    ));
  }

  sendOnOffReadEndpointCommand(deviceId: string, endPointId: string) : Observable<string> {
    const data = {
      nodeAlias: "No Value",
      nodeId: deviceId,
      endPointId: endPointId,
    };
    console.log(
      `Sending on/off read request to ${this.serverUrl}/onoff with data: ${JSON.stringify(data)}`
    );

    return this.httpClient.post<string>(
      `${this.serverUrl}/onoff_report`, JSON.stringify(data)).pipe(
      catchError((error: any) => {
        console.error('Error sending on/off read request', error);
        return throwError(error.message)
      }
    ));
  }

  sendBasicInformationCommand(nodeAlias: string, nodeId: string, endPointId: string) {
    const data = {
      nodeAlias: nodeAlias,
      nodeId: nodeId,
      endPointId: endPointId,
    };
    console.log(
      `Sending edit request to ${this.serverUrl}/basicinformation with data: ${JSON.stringify(data)}`
    );

    return this.httpClient.post<string>(
      `${this.serverUrl}/basicinformation`, JSON.stringify(data)).pipe(
      catchError((error: any) => {
        console.error('Error sending edit command', error);
        return throwError(error.message)
      }
    ));
  }

  // For Open Commissioning Window with BCM/ECM
  sendOpenCommissioningWindowWithBCM(nodeId: string, windowTimeout: string, option: string, iteration: string, discriminator: string) : Observable<string> {
    if(nodeId.length == 0 || windowTimeout.length == 0 || option.length == 0 || iteration.length == 0 || discriminator.length == 0) {
      return throwError("Please fill in all the input fields");
    }
    const data = {
      nodeId: nodeId,
      option: option,
      windowTimeout: windowTimeout,
      iteration: iteration,
      discriminator: discriminator,
    }
    if (data.option == "0") {
      console.log(`Sending Pairing open-commissioning window with BCM option to ${this.serverUrl}/multiadmin with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/multiadmin`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending multiadmin pairing BCM request', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("The option field is not 0, please correct the command or send with ECM (option = 1)")
    }
  }

  sendOpenCommissioningWindowWithECM(nodeId: string, windowTimeout: string, option: string, iteration: string, discriminator: string) : Observable<string> {
    if(nodeId.length == 0 || windowTimeout.length == 0 || option.length == 0 || iteration.length == 0 || discriminator.length == 0) {
      return throwError("Please fill in all the input fields");
    }
    const data = {
      nodeId: nodeId,
      option: option,
      windowTimeout: windowTimeout,
      iteration: iteration,
      discriminator: discriminator,
    }
    if (data.option == "1") {
      console.log(`Sending Pairing open-commissioning window with ECM option to ${this.serverUrl}/multiadmin with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/multiadmin`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending multiadmin pairing ECM request', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("The option field is not 1, please correct the command or send with BCM (option = 0)")
    }
  }

  // For EEVSE

  sendEEVSEEnableChargingCommand(nodeId: string, endpointId: string, chargingEnabledUntil: string, minimumChargeCurrent: string, maximumChargeCurrent: string, commandType: string) : Observable<string> {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && chargingEnabledUntil.length > 0 && minimumChargeCurrent.length > 0 && maximumChargeCurrent.length > 0;
    }

    const data = {
      nodeId: nodeId,
      endPointId: endpointId,
      type: commandType,
      chargingEnabledUntil: chargingEnabledUntil,
      minimumChargeCurrent: minimumChargeCurrent,
      maximumChargeCurrent: maximumChargeCurrent
    }

    if (fieldCheck()) {
      console.log(`Sending enable charging EEVSE command to ${this.serverUrl}/eevse_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/eevse_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending EEVSE enable charging command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId, chargingEnabledUntil, minimumChargeCurrent, maximumChargeCurrent")
    }
  }

  sendEEVSEWriteUserMaximumChargeCurrentCommand(nodeId: string, endpointId: string, userMaximumChargeCurrent: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && userMaximumChargeCurrent.length > 0;
    }
    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      userMaximumChargeCurrent: userMaximumChargeCurrent,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending write user maximum charge current EEVSE command to ${this.serverUrl}/eevse_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/eevse_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending EEVSE write max user charge current command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId, usermaximumchargecurrent)")
    }
  }

  sendEEVSEDisableChargingCommand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0;
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending EEVSE disable charging command to ${this.serverUrl}/eevse_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/eevse_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending EEVSE disable charging command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId)")
    }
  }


sendEEVSEReadCommand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheckCurrentState : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "state"
    }

    var fieldCheckSupplyState : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "supplystate"
    }

    var fieldCheckFaultState : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "faultstate"
    }

    var fieldCheckChargingEnabledUntil : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "chargingenableduntil"
    }

    var fieldCheckMinimumChargeCurrent : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "minimumchargecurrent"
    }

    var fieldCheckMaximumChargeCurrent : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "maximumchargecurrent"
    }

    var fieldCheckSessionId : Function = () => {

      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "sessionid"
    }

    var fieldCheckSessionDuration : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "sessionduration"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      nodeAlias: "No Value",
      type: commandType
    }

    if(fieldCheckCurrentState() || fieldCheckSupplyState() || fieldCheckFaultState() || fieldCheckChargingEnabledUntil() || fieldCheckMinimumChargeCurrent() || fieldCheckMaximumChargeCurrent() || fieldCheckSessionId() || fieldCheckSessionDuration()) {
      console.log(`Sending EEVSE read command to ${this.serverUrl}/eevse_read with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/eevse_read`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending EEVSE read command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }


  // For Media control read / control
  sendPlayAppMediaControlCommmand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "play"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending Media play command to ${this.serverUrl}/media_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media Play command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendPauseAppMediaControlCommmand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "pause"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending Media pause command to ${this.serverUrl}/media_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media pause command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendStopAppMediaControlCommmand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "stop"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending Media stop command to ${this.serverUrl}/media_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media stop command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendStartOverAppMediaControlCommmand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "startover"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending Media startover command to ${this.serverUrl}/media_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media start-over command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendPreviousAppMediaControlCommmand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "previous"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending Media previous command to ${this.serverUrl}/media_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media previous command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendNextAppMediaControlCommmand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "next"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending Media next command to ${this.serverUrl}/media_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media next command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendRewindAppMediaControlCommmand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "rewind"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending Media rewind command to ${this.serverUrl}/media_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media rewind command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendFastForwardAppMediaControlCommmand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheck : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "fastforward"
    }

    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheck()) {
      console.log(`Sending Media fastforward command to ${this.serverUrl}/media_control with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_control`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media fastforward command', error);
          return throwError(error.message);
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendOnMediaReadMediaControlCommand(nodeId: string, endpointId: string, commandType: string) {
    var fieldCheckCurrentState : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "currentstate"
    }

    var fieldCheckStartTime : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "starttime"
    }

    var fieldCheckDuration : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "duration"
    }

    var fieldCheckSampledPosition : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "sampledposition"
    }

    var fieldCheckPlaybackSpeedPosition : Function = () => {
      return nodeId.length > 0 && endpointId.length > 0 && commandType.length > 0 && commandType.toLowerCase() == "playbackspeed"
    }


    const data = {
      nodeId: nodeId,
      endpointId: endpointId,
      type: commandType
    }

    if(fieldCheckCurrentState() || fieldCheckStartTime() || fieldCheckDuration() || fieldCheckSampledPosition() || fieldCheckPlaybackSpeedPosition()) {
      console.log(`Sending Media read command to ${this.serverUrl}/media_read with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/media_read`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending Media read command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (nodeId, endpointId) and select the correct type")
    }
  }

  sendPayloadParsePostRequest(payload: string) {
    // Returns passcode, result
    var fieldCheck : Function = () => {
      return payload.length > 0
    }

    const data = {
      payload: payload
    }

    if (fieldCheck()) {
      console.log(`Sending Media read command to ${this.serverUrl}/payload_parse with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/payload_parse`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending payload parse command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields (payload).")
    }

  }

  sendTBRMCommissioningCommand(type: string, expLength: string, breadcrumb: string, nodeId: string, endpointId: string) {
    var fieldCheckArmFailsafe : Function = () => {
      return type.length > 0 && expLength.length > 0 && breadcrumb.length > 0 && nodeId.length > 0 && endpointId.length > 0 && type === "armfailsafe"
    }

    var fieldCheckCommissioningComplete : Function = () => {
      return type.length > 0 && nodeId.length > 0 && endpointId.length > 0 && type === "commissioningcomplete"
    }


    if (fieldCheckArmFailsafe() && !fieldCheckCommissioningComplete()) {
      const data = {
        type: type,
        expiryLengthSeconds: expLength,
        breadcrumb: breadcrumb,
        nodeId: nodeId,
        endPointId: endpointId
      }

      console.log(`Sending TBRM commissioning arm failsafe command to ${this.serverUrl}/generalcommissioning with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/generalcommissioning`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending TBRM arm failsafe commissioning command', error);
          return throwError(error.message)
        }
      ));

    } else if (fieldCheckCommissioningComplete() && !fieldCheckArmFailsafe()) {
      const data = {
        type: type,
        nodeId: nodeId,
        endPointId: endpointId
      }

      console.log(`Sending TBRM commissioning complete command to ${this.serverUrl}/generalcommissioning with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/generalcommissioning`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending TBRM commissioning complete command', error);
          return throwError(error.message)
        }
      ));

    } else {
      return throwError("Please complete all the missing input fields")
    }

  }

  sendTBRMCommissioningSetActiveDatasetCommand(dataset: string, nodeId: string, endpointId: string) {
    var fieldCheckSetActiveDataset : Function = () => {
      return dataset.length > 0 && nodeId.length > 0 && endpointId.length > 0
    }

    if (fieldCheckSetActiveDataset()) {
      const data = {
        type: "setactivedatasetrequest",
        dataset: dataset,
        nodeId: nodeId,
        endPointId: endpointId
      }

      console.log(`Sending TBRM commissioning set active dataset command to ${this.serverUrl}/threadborderroutermanagement with data: ${JSON.stringify(data)}`)
      return this.httpClient.post<string>(
        `${this.serverUrl}/threadborderroutermanagement`, JSON.stringify(data)).pipe(
        catchError((error: any) => {
          console.error('Error sending TBRM set active dataset commissioning command', error);
          return throwError(error.message)
        }
      ));
    } else {
      return throwError("Please complete all the missing input fields")
    }
  }

}

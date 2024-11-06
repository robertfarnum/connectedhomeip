import { PostRequestsService } from './../../services/post-requests.service';
import { HttpClient } from '@angular/common/http';
import { Component } from '@angular/core';
import { MatCardModule } from '@angular/material/card';
import { MatFormField, MatFormFieldModule, MatLabel } from '@angular/material/form-field';
import { MatIconModule } from '@angular/material/icon';
import { ScrollgallerycomponentComponent } from '../../mainapplicationbody/scrollgallerycomponent/scrollgallerycomponent.component';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { MatChip } from '@angular/material/chips';
import { FloatingactionareaComponent } from '../../mainapplicationbody/floatingactionarea/floatingactionarea.component';
import { SettingsapplicationComponent } from "../settingsapplication/settingsapplication.component";
import { LoaderService } from '../../services/loader.service';
import { AppDialogService } from '../../services/app-dialog.service';

@Component({
  selector: 'app-easyevseapplication',
  standalone: true,
  imports: [MatCardModule, MatIconModule,
    ScrollgallerycomponentComponent,
    MatFormField, MatLabel, MatFormFieldModule, MatInputModule, MatSelectModule, MatChip, FloatingactionareaComponent, SettingsapplicationComponent],
  templateUrl: './easyevseapplication.component.html',
  styleUrl: './easyevseapplication.component.css'
})
export class EasyevseapplicationComponent {
  constructor(private httpClient: HttpClient, private postRequestsService: PostRequestsService, private loaderService: LoaderService, private appDialogService: AppDialogService) {}

  onButtonPressedEventCatch(value: {
    buttonName: string;
    action: Function;
  }) {
    console.log('Button pressed event caught in the ApplicationBodyComponent; value: ' + value.buttonName);
    value.action();
  };

  // EEVSE Event trigger section
  startEEVSETriggerEvent(): void {

  }

  clearEEVSETriggerEvent(): void {

  }

  triggerEEVSEPluggedIn() : void {

  }

  clearEEVSEPluggedIn() : void {

  }

  triggerEEVSEChargingDemand() : void {

  }

  clearEEVSEChargingDemand() : void {
    console.log("Clear EEVSE Charging Demand")
  }

  // EEVSE enable charging section
  eevseEnableCharging() : void {
    console.log("EEVSE enabling charging")
    var nodeId = (document.getElementById("node-id") as (HTMLInputElement)).value
    var endpointId = (document.getElementById("endpoint-id") as (HTMLInputElement)).value
    var chargingEnabledUntil = (document.getElementById("charging-enabled-until") as (HTMLInputElement)).value
    var minimumChargeCurrent = (document.getElementById("minimum-charge-current") as (HTMLInputElement)).value
    var maximumChargeCurrent = (document.getElementById("maximum-charge-current") as (HTMLInputElement)).value

    this.postRequestsService.sendEEVSEEnableChargingCommand(nodeId, endpointId, chargingEnabledUntil, minimumChargeCurrent, maximumChargeCurrent, "enablecharging").subscribe(
      (data) => {
        console.log('Data received: ', data);
        this.loaderService.hideLoader();
        const parsedResult = JSON.parse(JSON.stringify(data));
        if (parsedResult.result === 'successful') {
          this.appDialogService.showInfoDialog('EEVSE enable charging command completed successfully');
        } else if (parsedResult.result === 'failed') {
          this.appDialogService.showErrorDialog('EEVSE enable charging command completed with errors');
        }
      },

      error => {
        console.error('Error received: ', error);
        this.loaderService.hideLoader();
        this.appDialogService.showErrorDialog('Error: '+ error);
      }
    )
  }

  // Write EEVSE User Maximum Charge Current Section
  writeEEVSEUserMaximumChargeCurrent() : void {
    console.log("Writing EEVSE User Maximum Charge Current")
    var nodeId = (document.getElementById("node-id") as (HTMLInputElement)).value
    var endpointId = (document.getElementById("endpoint-id") as (HTMLInputElement)).value
    var userMaximumChargeCurrent = (document.getElementById("user-maximum-charge-current") as (HTMLInputElement)).value

    this.postRequestsService.sendEEVSEWriteUserMaximumChargeCurrentCommand(nodeId, endpointId, userMaximumChargeCurrent, "write").subscribe(
      (data) => {
        console.log('Data received: ', data);
        this.loaderService.hideLoader();
        const parsedResult = JSON.parse(JSON.stringify(data));
        if (parsedResult.result === 'successful') {
          this.appDialogService.showInfoDialog('EEVSE write user maximum charge current command completed successfully');
        } else if (parsedResult.result === 'failed') {
          this.appDialogService.showErrorDialog('EEVSE write user maximum charge current completed with errors');
        }
      },

      error => {
        console.error('Error received: ', error);
        this.loaderService.hideLoader();
        this.appDialogService.showErrorDialog('Error: '+ error );
      }
    )
  }

  // EEVSE Disable Charging endpoint
  eevseDisableCharging() : void {
    console.log("Writing EEVSE disable charging command")
    var nodeId = (document.getElementById("node-id") as (HTMLInputElement)).value
    var endpointId = (document.getElementById("endpoint-id") as (HTMLInputElement)).value

    this.postRequestsService.sendEEVSEDisableChargingCommand(nodeId, endpointId, "disable").subscribe(
      (data) => {
        console.log('Data received: ', data);
        this.loaderService.hideLoader();
        const parsedResult = JSON.parse(JSON.stringify(data));
        if (parsedResult.result === 'successful') {
          this.appDialogService.showInfoDialog('EEVSE disable charging command completed successfully');
        } else if (parsedResult.result === 'failed') {
          this.appDialogService.showErrorDialog('EEVSE disable charging command completed with errors');
        }
      },

      error => {
        console.error('Error received: ', error);
        this.loaderService.hideLoader();
        this.appDialogService.showErrorDialog('Error: '+ error);
      }
    )
  }

  // EEVSE Status section
  onCurrentStateEEVSERead(command: string) : void {
    var values_for_the_input = {
      nodeId: (document.getElementById("node-id") as HTMLInputElement).value,
      endpointId: (document.getElementById("endpoint-id") as (HTMLInputElement)).value,
      nodeAlias: (document.getElementById("node-alias") as (HTMLInputElement)).value
    }

    this.loaderService.showLoader();

    interface ResponseIfce {
      report: string,
      result: string
    }

    this.postRequestsService.sendEEVSEReadCommand(
      values_for_the_input.nodeId, values_for_the_input.endpointId, command
    ).subscribe(
      (data: any) => {
        data = data as ResponseIfce;
        console.log('Data received: ', data);
        this.loaderService.hideLoader();
        const parsedResult = JSON.parse(JSON.stringify(data));
        if (parsedResult.result === 'successful') {
          this.appDialogService.showInfoDialog(`EEVSE read completed successfully. The report is ` + data.report);
        } else if (parsedResult.result === 'failed') {
          this.appDialogService.showErrorDialog('EEVSE read completed with errors');
        }
      },

      error => {
        console.error('Error received: ', error);
        this.loaderService.hideLoader();
        this.appDialogService.showErrorDialog('Error: '+ error);
      }
    );
  }
}

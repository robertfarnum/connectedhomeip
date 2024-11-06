import { PostRequestsService } from './../../services/post-requests.service';
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
  selector: 'app-tbrmappcomponent',
  standalone: true,
  imports: [MatCardModule, MatIconModule,
    ScrollgallerycomponentComponent,
    MatFormField, MatLabel, MatFormFieldModule, MatInputModule, MatSelectModule, MatChip, FloatingactionareaComponent, SettingsapplicationComponent],
  templateUrl: './tbrmappcomponent.component.html',
  styleUrl: './tbrmappcomponent.component.css'
})
export class TbrmappcomponentComponent {
  constructor(private loaderService: LoaderService, private postRequestsService: PostRequestsService, private appDialogService: AppDialogService) {}

  onButtonPressedEventCatch(value: {
    buttonName: string;
    action: Function;
  }) {
    console.log('Button pressed event caught in the ApplicationBodyComponent; value: ' + value.buttonName);
    value.action();
  };

  getInputFieldsCurrentValue() {
    return {
      type: (document.getElementById("type-input") as HTMLInputElement).value,
      expLength: (document.getElementById("explength-input") as HTMLInputElement).value,
      breadcrumb: (document.getElementById("breadcrumb-input") as HTMLInputElement).value,
      nodeId: (document.getElementById("nodeid-input") as HTMLInputElement).value,
      endpointId: (document.getElementById("endpointid-input") as HTMLInputElement).value,
    }

  }

  getInputFieldsGetActiveDatasetCurrentValue() {
    return {
      dataset: (document.getElementById("dataset-input") as HTMLInputElement).value,
      nodeId: (document.getElementById("nodeid2-input") as HTMLInputElement).value,
      endpointId: (document.getElementById("endpointid2-input") as HTMLInputElement).value,
    }
  }

  sendTBRMCommissioningArmDisarm() : void {
    console.log('TBRM commissioning arm / disarm command started');
    var values_for_the_input = this.getInputFieldsCurrentValue()
    this.loaderService.showLoader();

    this.postRequestsService.sendTBRMCommissioningCommand(
      values_for_the_input.type, values_for_the_input.expLength, values_for_the_input.breadcrumb, values_for_the_input.nodeId, values_for_the_input.endpointId
    ).subscribe(
      data => {
        console.log('Data received: ', data);
        this.loaderService.hideLoader();
        const parsedResult = JSON.parse(JSON.stringify(data));
        if (parsedResult.result === 'successful') {
          this.appDialogService.showInfoDialog('TBRM commissioning completed successfully');
        } else if (parsedResult.result === 'failed') {
          this.appDialogService.showErrorDialog('TBRM commissioning completed with errors');
        }
      },

      error => {
        console.error('Error received: ', error);
        this.loaderService.hideLoader();
        this.appDialogService.showErrorDialog('Error: '+ error);
      }
    );

  }

  sendTBRMCommissioningSetActiveDataset() : void {
    console.log('TBRM commissioning set active dataset command started');
    var values_for_the_input = this.getInputFieldsGetActiveDatasetCurrentValue()
    this.loaderService.showLoader();

    this.postRequestsService.sendTBRMCommissioningSetActiveDatasetCommand(
      values_for_the_input.dataset, values_for_the_input.nodeId, values_for_the_input.endpointId
    ).subscribe(
      data => {
        console.log('Data received: ', data);
        this.loaderService.hideLoader();
        const parsedResult = JSON.parse(JSON.stringify(data));
        if (parsedResult.result === 'successful') {
          this.appDialogService.showInfoDialog('TBRM commissioning set active dataset completed successfully');
        } else if (parsedResult.result === 'failed') {
          this.appDialogService.showErrorDialog('TBRM commissioning set active dataset completed with errors');
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

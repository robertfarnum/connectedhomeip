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

@Component({
  selector: 'app-bindingapplication',
  standalone: true,
  imports: [MatCardModule, MatIconModule,
    ScrollgallerycomponentComponent,
    MatFormField, MatLabel, MatFormFieldModule, MatInputModule, MatSelectModule, MatChip, FloatingactionareaComponent, SettingsapplicationComponent],
  templateUrl: './bindingapplication.component.html',
  styleUrl: './bindingapplication.component.css'
})
export class BindingapplicationComponent {
  onButtonPressedEventCatch(value: {
    buttonName: string;
    action: Function;
  }) {
    console.log('Button pressed event caught in the ApplicationBodyComponent; value: ' + value.buttonName);
    value.action();
  };
}

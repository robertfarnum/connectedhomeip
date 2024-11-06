import { Component, Directive, ElementRef, Input, ViewChild, OnInit, AfterViewInit, OnDestroy, Inject } from '@angular/core';
import { ApplicationcardComponent } from '../../applicationcard/applicationcard.component';
import { NgFor, NgIf, NgStyle } from '@angular/common';
import { CardModel } from '../../models/card-model';
import { NgxMasonryComponent, NgxMasonryModule } from 'ngx-masonry';

import { LoaderService } from './../../services/loader.service';
import { PostRequestsService } from './../../services/post-requests.service';
import { AppDialogWithInputFieldsService } from './../../services/app-dialog-input.service';
import { EventEmitter, Output } from '@angular/core';
import { MatChip, MatChipListbox } from '@angular/material/chips';
import { MatIcon } from '@angular/material/icon';
import {MatChipsModule} from '@angular/material/chips';
import { PopupdialogmaterialComponent } from '../../popupdialogmaterial/popupdialogmaterial.component';
import { MatDialog, MatDialogActions, MatDialogClose } from '@angular/material/dialog';
import { Dialog } from '@angular/cdk/dialog';
import {GetRequestsService} from '../../services/get-requests.service';

import { AppDialogService } from '../../services/app-dialog.service';
import { HttpClient } from '@angular/common/http';

@Component({
  selector: 'app-cardscomponent',
  standalone: true,
  imports: [ApplicationcardComponent, NgFor, NgxMasonryModule, NgStyle, NgIf,
    MatChip, MatChipListbox, MatIcon, MatChipsModule],
  templateUrl: './cardscomponent.component.html',
  styleUrl: './cardscomponent.component.css'
})
export class CardscomponentComponent implements AfterViewInit, OnDestroy {
  ngAfterViewInit(): void {
    throw new Error('Method not implemented.');
  }
  private MassonryLayoutGap = 20;

  constructor(private httpClient: HttpClient, private getRequestsService: GetRequestsService, private postRequestsService: PostRequestsService,
    private appDialogService: AppDialogService,
    private loaderService: LoaderService) {}

  // The cards that will be displayed in the cards component
  @Input() cards!: CardModel[];
  @Input() cardsHaveSubscriptionOption!: boolean;
  @ViewChild('ngxmasonry_container') masonrycontainer!: ElementRef;
  @ViewChild('masonry_item_container') masonryitemcontainer!: NgxMasonryComponent;
  @ViewChild('card_component_container') cardcomponentcontainer!: ElementRef;

  reloadMasonryLayout() {
    if (this.masonryitemcontainer !== undefined) {
      this.masonryitemcontainer.reloadItems();
      this.masonryitemcontainer.layout();
    }
  }

  ngOnDestroy(): void {
    console.log('Destroying Cards Component');
  }

  public getMasonryContainerWidth(): number {
    return this.masonrycontainer.nativeElement.offsetWidth;
  }


  public getCardComponentContainerWidth(): number {
    return this.cardcomponentcontainer.nativeElement.offsetWidth;
  }

  public getMasonryLayoutGap(): number {
    return this.MassonryLayoutGap;
  }

  public getNumberOfCards() {
    return this.cards.length;
  }

  public onPressedRefreshButton() {
    this.loaderService.showLoader();
    this.getRequestsService.getDevicesList().subscribe(
      (data: any) => {
        const parsedData = JSON.parse(JSON.stringify(data));
        if (parsedData['result'] === 'successful') {
          this.cards = [];
          for (const [key, value] of Object.entries(parsedData['devices'])) {
            let card: CardModel;
            const device = value as string;
            card = new CardModel('../../assets/wireless-transparent.png', parseInt(key), [], true, device);
            this.cards.push(card);
          }
          this.loaderService.hideLoader();
          // this.appDialogService.showInfoDialog('Got the list of devices successfully');
        } else {
          this.loaderService.hideLoader();
          this.appDialogService.showErrorDialog('Error getting devices list (id\'s and aliases)');
        }
      },
      (error: any) => {
        console.error('Error received: ', error);
        this.loaderService.hideLoader();
        this.appDialogService.showErrorDialog('Error getting devices list (id\'s and aliases)');
      }
  );
  }

}
